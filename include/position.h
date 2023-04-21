//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_POSITION_H
#define ARGUS_POSITION_H
#include <string>
#include <tsl/robin_map.h>

#include "trade.h"

using namespace std;

class Position{
private:
    ///is the position currently open
    bool is_open;

    ///unique id of the position
    unsigned int position_id;

    ///unique id of the underlying asset of the position
    string asset_id;

    ///how many units in the position
    double units;

    ///average price of the position
    double average_price;

    ///closing price of the position
    double close_price;

    ///last price the position was evaluated at
    double last_price;

    ///unrealized pl of the position
    double unrealized_pl;

    ///realized pl of the position
    double realized_pl;

    ///time the position was opened
    long long position_open_time;

    ///time the position was closed
    long long position_close_time;

    ///number of bars the positions has been held for
    unsigned int bars_held;

    tsl::robin_map<string,shared_ptr<Trade>> trades;
};

#endif //ARGUS_POSITION_H
