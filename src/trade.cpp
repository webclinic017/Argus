//
// Created by Nathan Tormaschy on 4/21/23.
//

#include <memory>

#include "order.h"
#include "trade.h"
#include "utils_array.h"
#include "settings.h"

using namespace std;

void Trade::cancel_child_order(unsigned int order_id)
{
    auto _order = unsorted_vector_remove(
        this->open_orders,
        [](const shared_ptr<Order> &obj)
        { return obj->get_order_id(); },
        order_id);
}

Trade::Trade(shared_ptr<Order> &filled_order, unsigned int trade_id_, Portfolio* source_portfolio_) : source_portfolio(source_portfolio_)
{
    // populate the ids
    this->trade_id = trade_id_;
    this->asset_id = filled_order->get_asset_id();
    this->exchange_id = filled_order->get_exchange_id();
    this->broker_id = filled_order->get_broker_id();
    this->portfolio_id = filled_order->get_portfolio_id();
    this->strategy_id = filled_order->get_strategy_id();

    // set the trade member variables
    this->units = filled_order->get_units();
    this->average_price = filled_order->get_average_price();
    this->unrealized_pl = 0;
    this->realized_pl = 0;
    this->close_price = 0;
    this->last_price = filled_order->get_average_price();

    // set the times
    this->trade_close_time = 0;
    this->trade_open_time = filled_order->get_fill_time();
    this->bars_held = 0;
}

void Trade::evaluate(double market_price, bool on_close)
{
    this->last_price = market_price;
    this->unrealized_pl = this->units * (market_price - this->average_price);
    if (on_close)
    {
        this->bars_held++;
    }
}

void Trade::adjust(shared_ptr<Order> filled_order)
{
    // extract order information
    auto units_ = filled_order->get_units();
    auto fill_price_ = filled_order->get_average_price();
    auto fill_time_ = filled_order->get_fill_time();

    // decided to close, increase, or reduce the trade
    if (units_ + this->units < 1e-8)
    {
        this->close(fill_price_, fill_time_);
    }
    else if (units_ * this->units > 0)
    {
        this->increase(fill_price_, units_, fill_time_);
    }
    else
    {
        this->reduce(fill_price_, units_, fill_time_);
    }
}

void Trade::close(double market_price_, long long int trade_close_time_)
{
    this->is_open = false;
    this->close_price = market_price_;
    this->trade_close_time = trade_close_time_;
    this->realized_pl += this->units * (market_price_ - this->average_price);
    this->unrealized_pl = 0;
}

void Trade::increase(double market_price, double units_, long long trade_change_time_)
{
#ifdef ARGUS_RUNTIME_ASSERT
    assert(this->is_open);              // assert trade is open
    assert(units + this->units > 1e-8); // assert not closing trade
    assert(units_ * this->units > 0);   // assert order was on same side
#endif
    double new_units = abs(this->units) + abs(units_);
    this->average_price = ((abs(this->units) * this->average_price) + (abs(units_) * market_price)) / new_units;
    this->units += units_;
    this->trade_change_time = trade_change_time_;
}
void Trade::reduce(double market_price_, double units_, long long int trade_change_time_)
{
#ifdef ARGUS_RUNTIME_ASSERT
    assert(this->is_open);              // assert trade is open
    assert(units + this->units > 1e-8); // assert note closing trade
    assert(units_ * this->units < 0);   // assert order was on different side
#endif
    this->realized_pl += abs(units_) * (market_price_ - this->average_price);
    this->trade_change_time = trade_change_time_;
    this->units += units_;
}

shared_ptr<Order> Trade::generate_order_inverse(){
    return std::make_shared<Order>(
        MARKET_ORDER,
        this->asset_id,
        this->units * -1,
        this->exchange_id,
        this->broker_id,
        this->portfolio_id,
        this->strategy_id,
        this->trade_id
    );
}
