//
// Created by Nathan Tormaschy on 4/21/23.
//
#pragma once
#ifndef ARGUS_ORDER_H
#define ARGUS_ORDER_H

class Trade;
class Portfolio;

#include <string>
#include <vector>
#include <memory>

using namespace std;

class Order;

enum OrderType
{
    MARKET_ORDER,
    LIMIT_ORDER,
    STOP_LOSS_ORDER,
    TAKE_PROFIT_ORDER
};

enum OrderState
{
    PENDING,  // order has been created but yet to be sent
    OPEN,     // order is open on the exchange
    FILLED,   // order has been filled by the exchange
    CANCELED, // order has been canceled by a strategy
};

enum OrderExecutionType
{
    EAGER, // order will be placed as soon as the broker gets it
    LAZY   // order will be placed in broker send orders sweep
};

enum OrderParentType
{
    TRADE, /// parent of the order is a smart pointer to a trade
    ORDER  /// parent of the order is a smart pointer to another open order
};

struct OrderParent
{
    /// type of parent for the order
    OrderParentType order_parent_type;

    /// union representing either a pointer to a trade or a order
    union
    {
        shared_ptr<Trade> parent_trade;
        shared_ptr<Order> parent_order;
    } member;
};

class OrderConsolidated{
public:
    //generate consolidated order
    OrderConsolidated(vector<shared_ptr<Order>> orders, Portfolio* source_portfolio);

    //get sp to parent order
    shared_ptr<Order> get_parent_order() const{ return this->parent_order;}

    vector<shared_ptr<Order>> & get_child_orders() { return this->child_orders;}

    //fill child orders using parent order fill;
    void fill_child_orders();

private: 
    /// @brief smart pointer to consildated parent order
    shared_ptr<Order> parent_order;

    /// @brief vector of smart pointer to child orders (never sent out)
    vector<shared_ptr<Order>> child_orders;

};

class Order
{
private:
    /// type of the order
    OrderType order_type;

    /// current state of the order;
    OrderState order_state;

    /// parent of the order. Used to link orders to be placed on conditional fill of another order
    /// or to link stop losses and take profits to existing trades
    OrderParent *order_parent;

    /// unique id of the portfolio that the order was placed to
    Portfolio* source_portfolio;

    /// child orders of the order to be placed on fill
    vector<shared_ptr<Order>> child_orders;

    /// unique id of the order
    unsigned int order_id;

    /// unique id of the trade the order is applied to
    int trade_id;

    /// number of units in the order
    double units;

    /// price the order was filled at
    double averae_price;

    /// time the order was filled
    long long order_fill_time;

    /// limit use for stop loss and take profit orders
    double limit;

    /// unique id of the underlying asset of the order
    string asset_id;

    /// unique id of the exchange that the asset is on
    string exchange_id;

    /// unique id of the broker to place the order to
    string broker_id;

    /// unique id of the strategy the order was placed by
    string strategy_id;

public:
    typedef shared_ptr<Order> order_sp_t;

    /// order constructor
    Order(OrderType order_type_, string asset_id_, double units_, string exchange_id_,
          string broker_id_, Portfolio* source_portfolio, string strategy_id_, int trade_id_);

    void cancel_child_order(unsigned int order_id);

    /// get the unique id of the order
    [[nodiscard]] unsigned int get_order_id() const { return this->order_id; }

    /// get the unique id of the exchange the order was placed to
    [[nodiscard]] string get_exchange_id() const { return this->exchange_id; }

    /// get the unique asset id of the order
    [[nodiscard]] string const & get_asset_id() const { return this->asset_id; }

    /// get the unique broker id of the broker the order was placed to
    [[nodiscard]] string const & get_broker_id() const { return this->broker_id; }

    /// get the unique strategy id of the strategy that placed the order
    [[nodiscard]] string const & get_strategy_id() const { return this->strategy_id; }

    /// get the unique portfolio id of the portfolio the order was placed to
    [[nodiscard]] Portfolio* get_source_portfolio() const { return this->source_portfolio; }
    
    /// get the unique trade id of the order
    [[nodiscard]] int get_trade_id() const { return this->trade_id; }

    /// convert trade id unsigned int, map -1 => 0 (base position trade)
    [[nodiscard]] unsigned int get_unsigned_trade_id() const;

    /// get the units in the order
    [[nodiscard]] double get_units() const { return this->units; }

    /// get the fill price in the order
    [[nodiscard]] double get_average_price() const { return this->averae_price; }

    /// get the limit of the order
    [[nodiscard]] double get_limit() const { return this->limit; }

    /// get the limit of the order
    [[nodiscard]] long long get_fill_time() const { return this->order_fill_time; }

    /// get the type of the order
    [[nodiscard]] OrderType get_order_type() const { return this->order_type; }

    /// get the state of the order
    [[nodiscard]] OrderState get_order_state() const { return this->order_state; }

    /// get pointer to an OrderParent struct containing information about the order's parent
    [[nodiscard]] OrderParent *get_order_parent() const;

    /// get reference to vector containing orders to be placed on fill
    [[nodiscard]] vector<shared_ptr<Order>> &get_child_orders() { return this->child_orders; }

    /// set the limit of the order
    inline void set_limit(double limit_) { this->limit = limit_; }

    inline void set_units(double units_) { this->units = units_;}

    // set the unique id of the order (broker sets when it is placed)
    inline void set_order_id(unsigned int order_id_) {this->order_id = order_id_;}
    
    /// set the trade id of the order
    inline void set_trade_id(unsigned int trade_id_) { this->trade_id = trade_id_; }

    /// set the order state of the order
    inline void set_order_state(OrderState order_state_) { this->order_state = order_state_; };
    
    /// fill the order at a given price and time
    void fill(double market_price, long long fill_time);
};

/// split a order into multiple sub orders based on number of units
shared_ptr<Order> split_order(shared_ptr<Order> existing_order, double new_order_units);

#endif // ARGUS_ORDER_H
