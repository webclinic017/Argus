//
// Created by Nathan Tormaschy on 4/22/23.
//

#ifndef ARGUS_HISTORY_H
#define ARGUS_HISTORY_H
#include <vector>
#include <list>
#include <unordered_map>
#include <memory>

#include "settings.h"
#include "trade.h"
#include "order.h"
#include "position.h"

/*
template <typename T>
class LinkedList {
public:
    // Add a new element to the beginning of the list
    void push_front(T element) {
        list_.push_front(element);
    }

    // Remove the first element from the list
    void pop_front() {
        list_.pop_front();
    }

private:
    std::list<T> list_;
};
*/

using namespace std;

class History{
private:
    /// history of all orders
    vector<shared_ptr<Order>> orders;

    /// history of all trades
    vector<shared_ptr<Trade>> trades;

    /// history of all positions
    //LinkedList<shared_ptr<Position>> positions;
    unordered_map<unsigned int, shared_ptr<Position>> positions;


public:
    vector<shared_ptr<Position>> test;

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
        this->positions.insert({position->get_position_id(), position});
    };
};


class PortfolioHistory{
    
};

#endif //ARGUS_HISTORY_H
