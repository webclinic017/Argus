//
// Created by Nathan Tormaschy on 4/21/23.
//
#pragma once
#ifndef ARGUS_TRADE_H
#define ARGUS_TRADE_H
#include <string>
#include <vector>
#include <memory>


#include "order.h"

using namespace std;

class Trade{
private:
    ///is the trade currently open
    bool is_open;

    ///unique id of the trade
    unsigned int trade_id;

    ///unique id of the underlying asset of the trade
    string asset_id;

    ///how many units in the trade
    double units;

    ///average price of the trade
    double average_price;

    ///closing price of the trade
    double close_price;

    ///last price the trade was evaluated at
    double last_price;

    ///unrealized pl of the trade
    double unrealized_pl;

    ///realized pl of the trade
    double realized_pl;

    ///time the trade was opened
    long long trade_open_time;

    ///time the trade was closed
    long long trade_close_time;

    ///number of bars the positions has been held for
    unsigned int bars_held;

    ///child orders currently open, used for take profit and stop loss orders
    vector<shared_ptr<Order>> open_orders;

public:
    ///trade constructor
    Trade(unsigned int trade_id, double units_, double average_price_, long long trade_create_time_);

    ///remove open order that has been canceled;
    shared_ptr<Order> cancel_child_order(unsigned int order_id);

};

#endif //ARGUS_TRADE_H
