//
// Created by Nathan Tormaschy on 4/23/23.
//
#include <memory>
#include <string>
#include <utility>

#include "portfolio.h"
#include "position.h"
#include "settings.h"
#include "utils_time.h"

Portfolio::Portfolio(int logging_, double cash_, string id_): positions_map(){
    this->logging = logging_;
    this->cash = cash_;
    this->portfolio_id = std::move(id_);
    this->position_counter = 0;
}

Portfolio::position_sp_t Portfolio::get_position(const string &asset_id) {
    auto iter = this->positions_map.find( asset_id );
    if ( this->positions_map.end() == iter ) {
        throw std::runtime_error( "Portfolio::get_position position does not exist" );
    }
    return iter->second;
}

void Portfolio::delete_position(const string &asset_id) {
    auto iter = this->positions_map.find( asset_id );
    if ( this->positions_map.end() == iter ) {
        throw std::runtime_error( "Portfolio::delete_position position does not exist" );
    };
}

void Portfolio::add_position(const string &asset_id, Portfolio::position_sp_t position) {
    auto iter = this->positions_map.find( asset_id );
    if ( this->positions_map.end() != iter ) {
        throw std::runtime_error( "Portfolio::add_position position already exists" );
    }
    this->positions_map.insert({asset_id, position});

}

void Portfolio::open_position(shared_ptr<Order> &filled_order) {
    //build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(filled_order, this->position_counter);
    this->position_counter++;

    //adjust cash held by broker accordingly
    this->cash -= filled_order->get_units() * filled_order->get_fill_price();

    //insert the new position into the portfolio object
    this->add_position(filled_order->get_asset_id(), position);

    //extract smart pointer to the new trade, so it can be passed on to account map and transform
    //trade id to unsigned int, -1 => 0 implies no trade id passed so set to position base trade
    auto trade_id = filled_order->get_unsigned_trade_id();
    auto trade = position->get_trade(trade_id);

    //get the account corresponding to the filled order and add trade to it
    auto account_id = filled_order->get_account_id();
    auto account = accounts->at(account_id);
    account.new_trade(trade);

#ifdef ARGUS_RUNTIME_ASSERT
        //assert trade pointer count is 3, one in the position, one in the account then one
    //in the local function scope
    assert(trade.use_count() == 3);
#endif
    //log the position if needed
    if(this->logging == 1){
        this->log_position_open(position);
    }
}

void Portfolio::close_position(shared_ptr<Order> &filled_order) {
    //get the position to close
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id);

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

        //push the trade to history
        this->history->remember_trade(std::move(trade));
    }

    //remove the position from master portfolio
    this->delete_position(asset_id);

    //push position to history
    this->history->remember_position(std::move(position));
}

void Portfolio::log_position_open(shared_ptr<Position>& new_position){
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  PORTFOLIO {}: POSITION {} AVERAGE PRICE AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               new_position->get_position_id(),
               new_position->get_average_price(),
               new_position->get_asset_id());
}