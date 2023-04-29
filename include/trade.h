//
// Created by Nathan Tormaschy on 4/21/23.
//
#pragma once
#include <cstddef>
#ifndef ARGUS_TRADE_H
#define ARGUS_TRADE_H

#include <string>
#include <vector>
#include <memory>

#include "order.h"

using namespace std;

class Portfolio;

class Trade
{

public:
    typedef shared_ptr<Trade> trade_sp_t;

    using order_sp_t = Order::order_sp_t;

    /// number of bars the positions has been held for
    unsigned int bars_held;

    /// trade constructor
    Trade(order_sp_t filled_order, bool dummy = false);

    /// get the location in memory of the trade
    auto get_mem_address(){return reinterpret_cast<std::uintptr_t>(this); }

    /// adjust a existing trade given a filled_order
    void adjust(order_sp_t filled_order);

    /// close the trade out at given time and price
    /// \param market_price price the trade was closed out at
    /// \param trade_close_time time the trade was closed out at
    void close(double market_price, long long trade_close_time);

    /// reduce trade side at given time and price
    /// \param market_price market price the adjustment order was filled at
    /// \param units number of units to reduce
    /// \param trade_change_time time the trade was changed
    void reduce(double market_price, double units, long long trade_change_time);

    /// increase trade side at given time and price
    /// \param market_price market price the adjustment order was filled at
    /// \param units number of units increase
    /// \param trade_change_time time the trade was changed
    void increase(double market_price, double units, long long trade_change_time);

    /// remove open order that has been canceled;
    void cancel_child_order(unsigned int order_id);

    /// get the id of the source portfolio of a trade
    /// @return ref to string of underlying source portfolio id 
    [[nodiscard]] Portfolio* get_source_portfolio() const { return this->source_portfolio; }

    /// is the trade currently open
    /// @return is the trade open
    [[nodiscard]] bool get_is_open() const { return this->is_open; }

    /// get number of units in the trade
    /// @return number of units in the trade
    [[nodiscard]] double get_units() const { return this->units; }

    /// get the time the trade was opened
    /// @return the time the trade opened
    [[nodiscard]] long long get_trade_open_time() const {return this->trade_open_time;}

    /// get the time the trade was closed
    /// @return the time the trade closed
    [[nodiscard]] long long get_trade_close_time() const {return this->trade_close_time;}

    /// get closing price of the trade
    /// @return closing price of the trade
    [[nodiscard]] double get_close_price() const { return this->close_price; }

    /// get the id of the underlying asset of the trade
    /// @return underlying asset of the trade
    [[nodiscard]] string const & get_asset_id() const { return this->asset_id; }

    /// get the id of the trade 
    /// @return id of the trade
    [[nodiscard]] unsigned int get_trade_id() const {return this->trade_id;}

    /// get the id of the underlying exchange of the trade
    /// @return ref to string of underlying exchange id 
    [[nodiscard]] string const & get_exchange_id() const { return this->exchange_id; }

    /// get the average price of the trade
    /// @return average price of the trade
    [[nodiscard]] double get_average_price() const { return this->average_price; }

    /// get the realized pl of the trade
    /// @return realized pl of the trade
    [[nodiscard]] double get_realized_pl() const { return this->realized_pl; }

    /// @brief get all open orders placed at the broker
    /// @return ref to vec of order smart pointers
    vector<order_sp_t> &get_open_orders() { return this->open_orders; }

    Portfolio* get_source_portfolio() {return this->source_portfolio;}

    /// @brief generate the inverse order needed to close out a trade, (MARKET_ORDER)
    /// @return a smart pointer to a order that when placed will close out the trade
    order_sp_t generate_order_inverse();

    /// set the trade source portfolio 
    /// @param pointer to source portfolio of the trade
    void set_source_portfolio(Portfolio* source_portfolio_) {this->source_portfolio = source_portfolio_;};

    double get_last_price(){return this->last_price;}
    double get_nlv(){return this->nlv;}
    double get_unrealized_pl(){return this->unrealized_pl;}
    void set_nlv(double nlv_){this->nlv = nlv_;}
    void set_last_price(double last_price_){this->last_price = last_price_;}
    void set_unrealized_pl(double unrealized_pl_){this->unrealized_pl = unrealized_pl_;}

private:
    /// static trade counter shared by all trade objects
    static inline unsigned int trade_counter = 0;

    /// is the trade currently open
    bool is_open;

    /// pointer to the source portfolio if the trade, i.e. the deepest portfolio in the
    /// portfolio tree to contain the trade that essentially "owns it"
    Portfolio* source_portfolio;

    /// unique id of the trade
    unsigned int trade_id;

    /// unique id of the underlying asset of the trade
    string asset_id;

    /// unique id of the exchange the underlying asset is on
    string exchange_id;

    /// unique id of the broker the trade was placed on
    string broker_id;

    /// unique id of the strategy that placed the order
    string strategy_id;

    /// net liquidation value of the trade
    double nlv;

    /// how many units in the trade
    double units;

    /// average price of the trade
    double average_price;

    /// closing price of the trade
    double close_price;

    /// last price the trade was evaluated at
    double last_price;

    /// unrealized pl of the trade
    double unrealized_pl;

    /// realized pl of the trade
    double realized_pl;

    /// time the trade was opened
    long long trade_open_time;

    /// time the trade was closed
    long long trade_close_time;

    /// time the trade was changed
    long long trade_change_time;

    /// child orders currently open, used for take profit and stop loss orders
    vector<shared_ptr<Order>> open_orders;
};

#endif // ARGUS_TRADE_H
