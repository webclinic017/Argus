//
// Created by Nathan Tormaschy on 4/21/23.
//
#pragma once
#ifndef ARGUS_ORDER_H
#define ARGUS_ORDER_H

class Trade;

#include <string>
#include <vector>
#include <memory>

using namespace std;

class Order;

enum OrderType {
    MARKET_ORDER,
    LIMIT_ORDER,
    STOP_LOSS_ORDER,
    TAKE_PROFIT_ORDER
};

enum OrderParentType{
    ///parent of the order is a smart pointer to a trade
    TRADE,

    ///parent of the order is a smart pointer to another open order
    ORDER
};

struct OrderParent{
    ///type of parent for the order
    OrderParentType order_parent_type;

    ///union representing either a pointer to a trade or a order
    union {
        shared_ptr<Trade> parent_trade;
        shared_ptr<Order> parent_order;
    } member;
};

class Order {
private:

    ///type of the order
    OrderType order_type;

    ///parent of the order. Used to link orders to be placed on conditional fill of another order
    ///or to link stop losses and take profits to existing trades
    OrderParent*  order_parent;

    ///child orders of the order to be placed on fill
    vector<shared_ptr<Order>> child_orders;

    ///unique id of the order
    unsigned int order_id{};

    ///number of units in the order
    double units;

    ///limit use for stop loss and take profit orders
    double limit{};

    ///unique id of the underlying asset of the order
    string asset_id;

    ///unique id of the exchange that the asset is on
    string exchange_id;

    ///unique id of the broker to place the order to
    string broker_id;

    ///unique id of the account that the order was placed to
    string account_id;

    ///unique id of the strategy the order was placed by
    string strategy_id;

public:
    ///order constructor
    Order(OrderType order_type_, string asset_id_, double units_,
          string exchange_id_, string broker_id_, string account_id_, string strategy_id_);

    shared_ptr<Order> cancel_child_order(unsigned int order_id);

    ///get the unique id of the order
    [[nodiscard]] unsigned int get_order_id() const {return this->order_id;}

    ///get the unique id of the exchange the order was placed to
    [[nodiscard]] string& get_exchange_id()  {return this->exchange_id;}

    ///get the unique asset id of the order
    [[nodiscard]] string get_asset_id() const {return this->asset_id;}

    ///get pointer to an OrderParent struct containing information about the order's parent
    [[nodiscard]] OrderParent* get_order_parent() const {return this->order_parent;}

    ///get reference to vector containing orders to be placed on fill
    [[nodiscard]] vector<shared_ptr<Order>>& get_child_orders() {return this->child_orders;}

    ///set the limit of the order
    void set_limit(double limit);

};

#endif //ARGUS_ORDER_H
