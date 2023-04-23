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

enum OrderState {
    PENDING,        //order has been created but yet to be sent
    OPEN,			//order is open on the exchange
    FILLED,			//order has been filled by the exchange
    CANCELED,		//order has been canceled by a strategy
};

enum OrderExecutionType{
    EAGER,          //order will be placed as soon as the broker gets it
    LAZY            //order will be placed in broker send orders sweep
};

enum OrderParentType{
    TRADE,          ///parent of the order is a smart pointer to a trade
    ORDER           ///parent of the order is a smart pointer to another open order
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

    ///current state of the order;
    OrderState order_state;

    ///parent of the order. Used to link orders to be placed on conditional fill of another order
    ///or to link stop losses and take profits to existing trades
    OrderParent*  order_parent;

    ///child orders of the order to be placed on fill
    vector<shared_ptr<Order>> child_orders;

    ///unique id of the order
    unsigned int order_id;

    ///unique id of the trade the order is applied to
    int trade_id;

    ///number of units in the order
    double units;

    ///price the order was filled at
    double fill_price;

    ///time the order was filled
    long long order_fill_time;

    ///limit use for stop loss and take profit orders
    double limit;

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
    Order(OrderType order_type_, string asset_id_, double units_,string exchange_id_,
          string broker_id_, string account_id_, string strategy_id_, int trade_id_ = -1);

    void cancel_child_order(unsigned int order_id);

    ///get the unique id of the order
    [[nodiscard]] unsigned int get_order_id() const {return this->order_id;}

    ///get the unique id of the exchange the order was placed to
    [[nodiscard]] string get_exchange_id()  {return this->exchange_id;}

    ///get the unique asset id of the order
    [[nodiscard]] string get_asset_id() {return this->asset_id;}

    ///get the unique trade id of the broker the order was placed to
    [[nodiscard]] string get_broker_id() const {return this->broker_id;}

    ///get the unique trade id of the account the order was placed to
    [[nodiscard]] string get_account_id() const {return this->account_id;}

    ///get the unique trade id of the order
    [[nodiscard]] int get_trade_id() const {return this->trade_id;}

    ///get the units in the order
    [[nodiscard]] double get_units() const {return this->units;}

    ///get the fill price in the order
    [[nodiscard]] double get_fill_price() const {return this->fill_price;}

    ///get the limit of the order
    [[nodiscard]] double get_limit() const {return this->limit;}

    ///get the limit of the order
    [[nodiscard]] long long get_fill_time() const {return this->order_fill_time;}

    ///get the type of the order
    [[nodiscard]] OrderType get_order_type() const {return this->order_type;}

    ///get the state of the order
    [[nodiscard]] OrderState get_order_state() const {return this->order_state;}

    ///get pointer to an OrderParent struct containing information about the order's parent
    [[nodiscard]] OrderParent* get_order_parent() const;

    ///get reference to vector containing orders to be placed on fill
    [[nodiscard]] vector<shared_ptr<Order>>& get_child_orders() {return this->child_orders;}

    ///set the limit of the order
    inline void set_limit(double limit_){this->limit = limit_;}

    ///set the order state of the order
    inline void set_order_state(OrderState order_state_){this->order_state = order_state_;};

    ///fill the order at a given price and time
    void fill(double market_price, long long fill_time);

};

#endif //ARGUS_ORDER_H