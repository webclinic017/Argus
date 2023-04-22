//
// Created by Nathan Tormaschy on 4/22/23.
//
#include <memory>
#include <string>
#include "order.h"
#include "position.h"


Position::Position(shared_ptr<Order>& filled_order, unsigned int position_id_){
    //position constructs to open state
    this->is_open = true;

    //set ids used by the position
    this->position_id = position_id_;
    this->asset_id = filled_order->get_asset_id();
    this->exchange_id = filled_order->get_exchange_id();

    // populate member variables
    this->units = filled_order->get_units();
    this->average_price = filled_order->get_fill_price();
    this->close_price = 0;
    this->last_price = filled_order->get_fill_price();
    this->unrealized_pl = 0;
    this->realized_pl = 0;

    // set time values
    this->position_open_time = filled_order->get_fill_time();
    this->position_close_time = 0;
    this->bars_held = 0;

    // if the trade id is -1 set it equal to 0 (the default trade container for a position)
    // else case the trade id to an unsigned int
    auto trade_id_int = filled_order->get_trade_id();
    if(trade_id_int == -1){trade_id_int++;}
    auto trade_id_uint = static_cast<unsigned int>(trade_id_int);

    //insert the new trade
    this->trades.insert({
        trade_id_uint,
        std::make_shared<Trade>(filled_order, filled_order->get_trade_id())
    });
}
