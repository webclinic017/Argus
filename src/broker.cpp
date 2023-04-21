//
// Created by Nathan Tormaschy on 4/21/23.
//
#include <string>
#include <memory>
#include <utility>

#include "broker.h"
#include "utils_array.h"

using namespace std;

Broker::Broker(string broker_id_, double cash_){
    this->broker_id = std::move(broker_id_);
    this->cash = cash_;
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

        //add the order to current open orders
        this->open_orders.push_back(order);
    }

    //clear the order buffer as all orders are now open
    this->open_orders_buffer.clear();
}
