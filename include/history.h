//
// Created by Nathan Tormaschy on 4/22/23.
//

#ifndef ARGUS_HISTORY_H
#define ARGUS_HISTORY_H
#include <vector>
#include <list>
#include <unordered_map>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

#include "settings.h"
#include "trade.h"
#include "order.h"
#include "position.h"
#include "utils_array.h"

using namespace std;

class History{
private:
    /// history of all orders
    vector<shared_ptr<Order>> orders;

    /// history of all trades
    vector<shared_ptr<Trade>> trades;

    /// history of all positions
    vector<shared_ptr<Position>> positions;

public:
    vector<shared_ptr<Position>> test;

    vector<shared_ptr<Order>>& get_order_history(){       return this->orders;   }
    vector<shared_ptr<Trade>>& get_trade_history(){       return this->trades;   }
    vector<shared_ptr<Position>>& get_position_history(){ return this->positions;}

    void remember_order(shared_ptr<Order> order){
#ifdef ARGUS_RUNTIME_ASSERT
        assert(order);
#endif
        this->orders.push_back(std::move(order));
    };

    void remember_trade(shared_ptr<Trade> trade){        
#ifdef ARGUS_RUNTIME_ASSERT
        assert(trade);
        assert(trade->get_trade_id() >= 0);
#endif
        this->trades.emplace_back(trade);
    };

    void remember_position(shared_ptr<Position> position){
#ifdef ARGUS_RUNTIME_ASSERT
        assert(position);
#endif
        this->positions.emplace_back(position);
    };
};

#endif //ARGUS_HISTORY_H
