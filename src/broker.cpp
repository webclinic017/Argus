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

void Broker::build(
        Exchanges* exchanges_,
        Portfolio* portfolio_,
        Accounts* accounts_)
{
    this->exchanges = exchanges_;
    this->portfolio = portfolio_;
    this->accounts = accounts_;
}

void Broker::cancel_order(unsigned int order_id) {
    auto order = unsorted_vector_remove(
            this->open_orders,
            [](const shared_ptr<Order> &obj) { return obj->get_order_id(); },
            order_id
    );

    //set the order state to cancel
    order->set_order_state(CANCELED);

    //get the order's parent if exists
    auto order_parent_struct = order->get_order_parent();

    //if the order has no parent then return
    if(!order_parent_struct){
        //move canceled order to history
        this->history->remember_order(std::move(order));
        return;
    }

    //remove the open order from the open order's parent
    switch (order_parent_struct->order_parent_type) {
        case TRADE: {
            auto trade = order_parent_struct->member.parent_trade;
            trade->cancel_child_order(order_id);
        }
        case ORDER: {
            auto parent_order = order_parent_struct->member.parent_order;
            parent_order->cancel_child_order(order_id);
        }
    }

    //recursively cancel all child orders of the canceled order
    for(auto& child_orders : order->get_child_orders()){
        this->cancel_order(child_orders->get_order_id());
    }

    //move canceled order to history
    this->history->remember_order(std::move(order));
}

void Broker::place_order(shared_ptr<Order>& order) {
    //get smart pointer to the right exchange
    auto exchange = this->exchanges->at(order->get_exchange_id());

    //send the order
    exchange->place_order(order);

    if(order->get_order_state() == FILLED){
        this->process_filled_order(order);
    }
}

void Broker::place_market_order(const string& asset_id_, double units_,
                        const string& exchange_id_,
                        const string& account_id_,
                        const string& strategy_id_,
                        OrderExecutionType order_execution_type){
    //build new smart pointer to shared order
    auto market_order =  make_shared<Order>
            (MARKET_ORDER,
             asset_id_,
             units_,
             exchange_id_,
             this->broker_id,
             account_id_,
             strategy_id_);

    if(order_execution_type == EAGER){
        //place order directly and process
        this->place_order(market_order);
    }
    else {
        //push the order to the buffer that will be processed when the buffer is flushed
        this->open_orders_buffer.push_back(market_order);
    }
}

void Broker::place_limit_order(const string& asset_id_, double units_, double limit_,
                                const string& exchange_id_,
                                const string& account_id_,
                                const string& strategy_id_,
                               OrderExecutionType order_execution_type){
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

    if(order_execution_type == EAGER){
        //place order directly and process
        this->place_order(limit_order);
    }
    else {
        //push the order to the buffer that will be processed when the buffer is flushed
        this->open_orders_buffer.push_back(limit_order);
    }
}

void Broker::send_orders(){
    //send orders from buffer to the exchange
    for(auto & order : this->open_orders_buffer){
        //get the exchange the order was placed to
        auto exchange = exchanges->at(order->get_exchange_id());

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
        shared_ptr<Order> &filled_order
        ){
    //get the position to close
    auto asset_id = filled_order->get_asset_id();
    auto position = portfolio->at(asset_id);

    //adjust cash held at the broker
    this->cash += filled_order->get_units() * filled_order->get_fill_price();

    //close all child trade of the position whose broker is equal to current broker id
    auto trades = position->get_trades();
    for(auto it = trades.begin(); it != trades.end(); ) {
        auto trade = it->second;

        //close the child trade
        trade->close(filled_order->get_fill_price(), filled_order->get_fill_time());

        //adjust the corresponding account and remove the trade from its portfolio
        auto account = &accounts->at(filled_order->get_account_id());
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
    portfolio->erase(asset_id);

    //push position to history
    this->history->remember_position(std::move(position));
}

void Broker::open_position(shared_ptr<Order>& filled_order){
    //build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(filled_order, this->position_counter);
    this->position_counter++;

    //adjust cash held by broker accordingly
    this->cash -= filled_order->get_units() * filled_order->get_fill_price();

    //insert the new position into the master portfolio object
    portfolio->insert({filled_order->get_asset_id(), position});

    //extract smart pointer to the new trade, so it can be passed on to account map and transform
    //trade id to unsigned int, -1 => 0 implies no trade id passed so set to position base trade
    auto trade_id_int = filled_order->get_trade_id();
    if(trade_id_int == -1){trade_id_int++;}
    auto trade_id_uint = static_cast<unsigned int>(trade_id_int);
    auto trade = position->get_trade(trade_id_uint);

    //get the account corresponding to the filled order
    auto account_id = filled_order->get_account_id();
    auto account = accounts->at(account_id);

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

void Broker::process_filled_order(shared_ptr<Order>& open_order) {
    //log the order if needed
    if (this->logging == 1) {
        this->log_order_fill(open_order);
    }

    //no position exists in the portfolio with the filled order's asset_id
    if (!portfolio->contains(open_order->get_asset_id())) {
        this->open_position(open_order);
    } else {
        auto position = portfolio->at(open_order->get_asset_id());
        //filled order is not closing existing position
        if (position->get_units() + open_order->get_units() > 1e-7) {
            this->modify_position(open_order);
        }
            //filled order is closing an existing position
        else {
            this->close_position(open_order);
        }
    }

    //place child orders from the filled order
    for(auto& child_order : open_order->get_child_orders()){
        this->place_order(child_order);
    }
}

void Broker::process_orders(){
    //iterate over open orders and process and filled ones
    for (auto it =  this->open_orders.begin(); it !=  this->open_orders.end(); ){
        auto order = *it;

        if(order->get_order_state() != FILLED){
            it++;
            continue;
        }
        else{
            //process the individual order
            this->process_filled_order(order);

            //push the filled order to the history
            this->history->remember_order(std::move(order));

            //remove filled order and move to next open order
            it = this->open_orders.erase(it);
        }
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

