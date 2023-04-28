//
// Created by Nathan Tormaschy on 4/21/23.
//
#include "order.h"

#include "settings.h"
#include "utils_array.h"
#include <cstddef>
#include <cstdio>

OrderConsolidated::OrderConsolidated(vector<shared_ptr<Order>> orders, Portfolio* source_portfolio){
    double units_ = 0;

    //validate at least 1 order
    assert(orders.size() > 0);

    //get the first asset id (all must match)
    auto order = orders[0];
    auto asset_id_ = order->get_asset_id();
    auto broker_id_ = order->get_broker_id();

    for(auto const &order : orders){
        units_ += order->get_units();

        #ifdef ARGUS_RUNTIME_ASSERT
        //orders must have same asset id
        assert(order->get_asset_id() == asset_id_);

        //orders must have same broker id
        assert(order->get_broker_id() == broker_id_);
        
        //orders must be market orders
        assert(order->get_order_type() == MARKET_ORDER);
        #endif
    }
    
    this->parent_order = make_shared<Order>(MARKET_ORDER,
            asset_id_,
            units_,
            order->get_exchange_id(),
            broker_id_,
            source_portfolio,
            "master",
            0);

    this->child_orders = std::move(orders);
}

void OrderConsolidated::fill_child_orders(){
    #ifdef ARGUS_RUNTIME_ASSERT
    //validate parent order was filled
    assert(this->parent_order->get_order_state() == FILLED);
    #endif

    auto market_price = parent_order->get_average_price();
    auto fill_time = parent_order->get_fill_time();

    for(auto& child_order : this->child_orders){
        child_order->fill(market_price, fill_time);
    }
}


Order::Order(OrderType order_type_, string asset_id_, double units_, string exchange_id_,
             string broker_id_, Portfolio* source_portfolio, string strategy_id_, int trade_id_)
{
    this->order_type = order_type_;
    this->units = units_;
    this->averae_price = 0.0;
    this->order_fill_time = 0;

    // populate the ids of the order
    this->asset_id = asset_id_;
    this->exchange_id = exchange_id_;
    this->broker_id = broker_id_;
    this->strategy_id = strategy_id_;

    assert(source_portfolio);    
    this->source_portfolio = source_portfolio;

    // set the order id to 0 (it will be populated by the broker object who sent it
    this->order_id = 0;

    // set the limit to 0, broker will populate of the order type is tp or sl
    this->limit = 0;

    // set the trade id (-1 default value implies use the default trade of a position
    this->trade_id = trade_id_;

    // set the order state equal to PENDING (yet to be place)
    this->order_state = PENDING;

    // set the order parent to nullptr at first
    this->order_parent = nullptr;
}

void Order::fill(double market_price, long long fill_time)
{
    this->averae_price = market_price;
    this->order_fill_time = fill_time;
    this->order_state = FILLED;
}

void Order::cancel_child_order(unsigned int order_id_)
{
    auto _order = unsorted_vector_remove(
        this->child_orders,
        [](const shared_ptr<Order> &obj)
        { return obj->get_order_id(); },
        order_id_);
}

unsigned int Order::get_unsigned_trade_id() const
{
    auto trade_id_int = this->trade_id;
    if (trade_id_int == -1)
    {
        trade_id_int++;
    }
    auto trade_id_uint = static_cast<unsigned int>(trade_id_int);
    return trade_id_int;
};

OrderParent *Order::get_order_parent() const
{
#ifdef ARGUS_RUNTIME_ASSERT
    // asser that order parent is not nullptr
    assert(this->order_parent);
#endif
    return this->order_parent;
}
