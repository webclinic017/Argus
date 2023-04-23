//
// Created by Nathan Tormaschy on 4/21/23.
//
#include "order.h"

#include "settings.h"
#include "utils_array.h"

void Order::cancel_child_order(unsigned int order_id_)
{
    auto _order = unsorted_vector_remove(
        this->child_orders,
        [](const shared_ptr<Order> &obj)
        { return obj->get_order_id(); },
        order_id_);
}

Order::Order(OrderType order_type_, string asset_id_, double units_, string exchange_id_,
             string broker_id_, string portfolio_id_, string strategy_id_, int trade_id_)
{

    this->order_type = order_type_;
    this->units = units_;
    this->fill_price = 0.0;
    this->order_fill_time = 0;

    // populate the ids of the order
    this->asset_id = std::move(asset_id_);
    this->exchange_id = std::move(exchange_id_);
    this->portfolio_id = std::move(portfolio_id_);
    this->broker_id = std::move(broker_id_);
    this->strategy_id = std::move(strategy_id_);

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
    this->fill_price = market_price;
    this->order_fill_time = fill_time;
    this->order_state = FILLED;
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
