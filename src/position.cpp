//
// Created by Nathan Tormaschy on 4/22/23.
//
#include <memory>
#include <string>
#include "order.h"
#include "position.h"
#include "settings.h"


Position::Position(trade_sp_t trade){
    //populate common position values
    this->populate_position(trade);

    // populate order values
    this->average_price = trade->get_average_price();
    this->last_price = trade->get_average_price();
    this->position_open_time = trade->get_trade_open_time();

    // insert the new trade
    this->trades.insert({trade->get_trade_id(),trade});
    this->trade_counter = 1;
};

Position::Position(shared_ptr<Order> &filled_order, unsigned int position_id_)
{   
    //populate common position values
    this->populate_position(filled_order);

    // set ids used by the position
    this->position_id = position_id_;

    // populate order values
    this->average_price = filled_order->get_fill_price();
    this->last_price = filled_order->get_fill_price();
    this->position_open_time = filled_order->get_fill_time();

    // if the trade id is -1 set it equal to 0 (the default trade container for a position)
    // else case the trade id to an unsigned int
    auto trade_id_int = filled_order->get_trade_id();
    if (trade_id_int == -1)
    {
        trade_id_int++;
    }
    auto trade_id_uint = static_cast<unsigned int>(trade_id_int);

    // insert the new trade
    this->trades.insert({trade_id_uint,
                         std::make_shared<Trade>(filled_order, filled_order->get_unsigned_trade_id())});
    this->trade_counter = 1;
}

shared_ptr<Trade> Position::adjust(shared_ptr<Order> &filled_order)
{
#ifdef ARGUS_RUNTIME_ASSERT
    assert(this->is_open);
    assert(units + this->units > 1e-8); // assert not closing trade
#endif

    // adjust the parent position's members
    auto units_ = filled_order->get_units();
    auto fill_price = filled_order->get_fill_price();

    // increasing position
    if (units_ * this->units > 0)
    {
        double new_units = abs(this->units) + abs(units_);
        this->average_price = ((abs(this->units) * this->average_price) + (abs(units_) * fill_price)) / new_units;
    }
    // reducing position
    else
    {
        this->realized_pl += abs(units_) * (fill_price - this->average_price);
    }

    // adjust position units
    this->units += units_;

    // get the trade id unsigned, and signed raw (-1 means open new trade)
    auto trade_id_uint = filled_order->get_unsigned_trade_id();
    auto trade_id_int = filled_order->get_trade_id();

    // no trade id was passed, open new trade
    if (trade_id_int == -1)
    {
        this->trades.insert({trade_id_uint,
                             std::make_shared<Trade>(filled_order, this->trade_counter)});
        this->trade_counter++;
        return this->trades.at(this->trade_counter - 1);
    }

    // trade id was passed but is not in position's child trade map so create new trade
    else if (!this->trades.contains(trade_id_uint))
    {
        this->trades.insert({trade_id_uint,
                             std::make_shared<Trade>(filled_order, filled_order->get_trade_id())});
        this->trade_counter++;
        return this->trades.at(this->trade_counter - 1);
    }

    // found the trade currently open
    else
    {
        auto trade = this->trades[trade_id_uint];
        trade->adjust(filled_order);

        // if the adjustment causes the trade to close remove from position child trades
        if (!trade->get_is_open())
        {
            this->trades.erase(trade_id_uint);
        }

        return trade;
    }
}

void Position::close(double market_price_, long long int position_close_time_)
{
    // close the position
    this->is_open = false;
    this->close_price = market_price_;
    this->position_close_time = position_close_time_;
    this->realized_pl += this->units * (market_price_ - this->average_price);
    this->unrealized_pl = 0;

    // close each of the individual child trades
    for (auto &trade_pair : this->trades)
    {
        auto &existing_trade = trade_pair.second;
        existing_trade->close(close_price, position_close_time);
    }
}
