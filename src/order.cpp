//
// Created by Nathan Tormaschy on 4/21/23.
//
#include "order.h"

#include <utility>
#include "utils_array.h"

shared_ptr<Order> Order::cancel_child_order(unsigned int order_id) {
    auto order = unsorted_vector_remove(
            this->child_orders,
            [](const shared_ptr<Order> &obj) { return obj->get_order_id(); },
            order_id
    );
    return order;
}

Order::Order(OrderType order_type_, string asset_id_, double units_,
             string exchange_id_, string broker_id_, string account_id_, string strategy_id_) {
    this->order_type = order_type_;
    this->units = units_;

    //populate the ids of the order
    this->asset_id = std::move(asset_id_);
    this->exchange_id = std::move(exchange_id_);
    this->account_id = std::move(account_id_);
    this->broker_id = std::move(broker_id_);
    this->strategy_id = std::move(strategy_id_);

    //set the order id to 0 (it will be populated by the broker object who sent it
    this->order_id = 0;

    //set the limit to 0, broker will populate of the order type is tp or sl
    this->limit = 0;

    this->order_parent = nullptr;
}

void Order::set_limit(double limit_) {
    if(this->order_id == MARKET_ORDER){
        throw std::runtime_error("attempting to set limit on market order");
    }
    else{
        this->limit = limit_;
    }
}

