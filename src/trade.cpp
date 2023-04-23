//
// Created by Nathan Tormaschy on 4/21/23.
//

#include <memory>
#include <string>

#include "order.h"
#include "trade.h"
#include "utils_array.h"

using namespace std;

shared_ptr<Order> Trade::cancel_child_order(unsigned int order_id) {
    auto order = unsorted_vector_remove(
            this->open_orders,
            [](const shared_ptr<Order> &obj) { return obj->get_order_id(); },
            order_id
    );
    return order;
}

Trade::Trade(shared_ptr<Order>& filled_order, unsigned int trade_id_) {
    //populate the ids
    this->trade_id = trade_id_;
    this->asset_id = filled_order->get_asset_id();
    this->exchange_id = filled_order->get_exchange_id();
    this->broker_id = filled_order->get_broker_id();
    this->account_id = filled_order->get_account_id();

    //set the trade member variables
    this->units = filled_order->get_units();
    this->average_price = filled_order->get_fill_price();
    this->unrealized_pl = 0;
    this->realized_pl = 0;
    this->close_price = 0;
    this->last_price = filled_order->get_fill_price();

    //set the times
    this->trade_close_time = 0;
    this->trade_open_time = filled_order->get_fill_time();
    this->bars_held = 0;
}

void Trade::evaluate(double market_price, bool on_close) {
    this->last_price = market_price;
    this->unrealized_pl = this->units * (market_price - this->average_price);
    if(on_close){
        this->bars_held++;
    }
}

void Trade::close(double market_price_, long long int trade_close_time_) {
    this->is_open = false;
    this->close_price = market_price_;
    this->trade_close_time = trade_close_time_;
    this->realized_pl += this->units * (market_price_ - this->average_price);
    this->unrealized_pl = 0;
}
