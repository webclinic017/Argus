//
// Created by Nathan Tormaschy on 4/21/23.
//
#include <string>
#include <memory>
#include <utility>
#include <fmt/core.h>

#include "broker.h"
#include "position.h"
#include "settings.h"
#include "utils_array.h"
#include "utils_time.h"

using namespace std;

Broker::Broker(string broker_id_, double cash_, int logging_, shared_ptr<History> history_){
    this->history = std::move(history_);
    this->broker_id = std::move(broker_id_);
    this->cash = cash_;
    this->logging = logging_;
    this->order_counter = 0;
    this->position_counter = 0;
}

shared_ptr<Order> Broker::cancel_order(unsigned int order_id) {
    auto order = unsorted_vector_remove(
            this->open_orders,
            [](const shared_ptr<Order> &obj) { return obj->get_order_id(); },
            order_id
    );

    //get the order's parent if exists
    auto order_parent_struct = order->get_order_parent();

    //if the order has no parent then return
    if(!order_parent_struct){
        return order;
    }

    //remove the open order from the open order's parent
    switch (order_parent_struct->order_parent_type) {
        case TRADE: {
            auto trade = order_parent_struct->member.parent_trade;
            auto child_order = trade->cancel_child_order(order_id);
        }
        case ORDER: {
            auto parent_order = order_parent_struct->member.parent_order;
            auto child_order = parent_order->cancel_child_order(order_id);
        }
    }

    //recursively cancel all child orders of the canceled order
    for(auto& child_orders : order->get_child_orders()){
        this->cancel_order(child_orders->get_order_id());
    }

    return order;
}

void Broker::place_market_order(const string& asset_id_, double units_,
                        const string& exchange_id_,
                        const string& account_id_,
                        const string& strategy_id_){
    //build new smart pointer to shared order
    auto market_order =  make_shared<Order>
            (MARKET_ORDER,
             asset_id_,
             units_,
             exchange_id_,
             this->broker_id,
             account_id_,
             strategy_id_);

    //push the order to the buffer that will be processed when the buffer is flushed
    this->open_orders_buffer.push_back(market_order);
}

void Broker::place_limit_order(const string& asset_id_, double units_, double limit_,
                                const string& exchange_id_,
                                const string& account_id_,
                                const string& strategy_id_){
    //build new smart pointer to shared order
    auto limit_order =  make_shared<Order>
            (LIMIT_ORDER,
             asset_id_,
             units_,
             exchange_id_,
             this->broker_id,
             account_id_,
             strategy_id_);

    //set the limit of the order
    limit_order->set_limit(limit_);

    //push the order to the buffer that will be processed when the buffer is flushed
    this->open_orders_buffer.push_back(limit_order);
}

void Broker::send_orders( tsl::robin_map<string,shared_ptr<Exchange>>& exchanges){
    //send orders from buffer to the exchange
    for(auto & order : this->open_orders_buffer){
        //get the exchange the order was placed to
        auto exchange = exchanges.at(order->get_exchange_id());

        //send order to rest on the exchange
        exchange->place_order(order);

        //set the order to state to open as it is now open on exchange
        order->set_order_state(OPEN);

        //add the order to current open orders
        this->open_orders.push_back(order);
    }

    //clear the order buffer as all orders are now open
    this->open_orders_buffer.clear();
}

void Broker::modify_position(shared_ptr<Order>&filled_order){}

void Broker::close_position(
        Portfolio &portfolio,
        Accounts &accounts,
        shared_ptr<Order> &filled_order
        ){
    //get the position to close
    auto asset_id = filled_order->get_asset_id();
    auto position = portfolio.at(asset_id);

    //adjust cash held at the broker
    this->cash += filled_order->get_units() * filled_order->get_fill_price();

    //close all child trade of the position whose broker is equal to current broker id
    auto trades = position->get_trades();
    for(auto it = trades.begin(); it != trades.end(); ) {
        auto trade = it->second;

        //close the child trade
        trade->close(filled_order->get_fill_price(), filled_order->get_fill_time());

        //adjust the corresponding account and remove the trade from its portfolio
        auto account = &accounts.at(filled_order->get_account_id());
        account->close_trade(filled_order->get_asset_id());

        //remove the trade from the position container
        it = trades.erase(it);

        //cancel orders whose parent is the closed trade
        for(auto &order : trade->get_open_orders()){
            this->cancel_order(order->get_order_id());
        }

        //push the trade to history
        this->history->remember_trade(std::move(trade));
    }

    //remove the position from master portfolio
    portfolio.erase(asset_id);

    //push position to history
    this->history->remember_position(std::move(position));
}

void Broker::open_position(
        Portfolio& portfolio,
        Account& account,
        shared_ptr<Order>&filled_order
        ){
    //build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(filled_order, this->position_counter);
    this->position_counter++;

    //adjust cash held by broker accordingly
    this->cash -= filled_order->get_units() * filled_order->get_fill_price();

    //insert the new position into the master portfolio object
    portfolio.insert({filled_order->get_asset_id(), position});

    //extract smart pointer to the new trade, so it can be passed on to account map and transform
    //trade id to unsigned int, -1 => 0 implies no trade id passed so set to position base trade
    auto trade_id_int = filled_order->get_trade_id();
    if(trade_id_int == -1){trade_id_int++;}
    auto trade_id_uint = static_cast<unsigned int>(trade_id_int);
    auto trade = position->get_trade(trade_id_uint);
    account.new_trade(trade);

    //log the position if needed
    if(this->logging == 1){
        this->log_position_open(position);
    }

#ifdef ARGUS_RUNTIME_ASSERT
    //assert trade pointer count is 3, one in the position, one in the account then one
    //in the local function scope
    assert(trade.use_count() == 3);
#endif

}

void Broker::process_orders(
        tsl::robin_map<string,shared_ptr<Position>>& portfolio,
        tsl::robin_map<string, Account> &accounts
        ){
    vector<size_t> filled_order_indecies;
    size_t index = 0;
    for(auto& open_order : this->open_orders){
        if(open_order->get_order_state() == FILLED){

            //log the order if needed
            if(this->logging == 1){
                this->log_order_fill(open_order);
            }

            //get the account corresponding to the filled order
            auto account_id = open_order->get_account_id();
            auto account = accounts.at(account_id);

            //no position exists in the portfolio with the filled order's asset_id
            if(!portfolio.contains(open_order->get_asset_id())){
                this->open_position(portfolio, account, open_order);
            }
            else{
                auto position = portfolio.at(open_order->get_asset_id());
                //filled order is not closing existing position
                if(position->get_units() + open_order->get_units() > 1e-7){
                    this->modify_position(open_order);
                }
                //filled order is closing an existing position
                else{
                    this->close_position(portfolio, accounts, open_order);
                }
            }
            filled_order_indecies.push_back(index);
        }
        index++;
    }
    //remove filled orders
    for(auto order_index : filled_order_indecies){
        //pop filled order off of the back
        std::swap(this->open_orders[order_index], this->open_orders.back());
        auto filled_order = this->open_orders.back();
        this->open_orders.pop_back();

        //push the filled order to the history
        this->history->remember_order(std::move(filled_order));
    }
};

void Broker::log_order_fill(shared_ptr<Order>& filled_order){
    auto datetime_str = nanosecond_epoch_time_to_string(filled_order->get_fill_time());
    fmt::print("{}:  BROKER {}: ORDER {} FILLED AT {}, ASSET_ID: {}",
               datetime_str,
               this->broker_id,
               filled_order->get_order_id(),
               filled_order->get_fill_price(),
               filled_order->get_asset_id());
};

void Broker::log_position_open(shared_ptr<Position>& new_position){
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  BROKER {}: POSITION {} AVERAGE PRICE AT {}, ASSET_ID: {}",
               datetime_str,
               this->broker_id,
               new_position->get_position_id(),
               new_position->get_average_price(),
               new_position->get_asset_id());
}

