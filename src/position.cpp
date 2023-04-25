//
// Created by Nathan Tormaschy on 4/22/23.
//
#include <memory>
#include <string>
#include "order.h"
#include "position.h"
#include "settings.h"
#include "trade.h"

using order_sp_t = Order::order_sp_t;

Position::Position(trade_sp_t trade, unsigned int trade_id, Portfolio* source_portfolio){
    //populate common position values
    this->asset_id = trade->get_asset_id();
    this->exchange_id = trade->get_exchange_id();
    this->units = trade->get_units();

    // populate order values
    this->is_open = true;
    this->average_price = trade->get_average_price();
    this->last_price = trade->get_average_price();
    this->position_open_time = trade->get_trade_open_time();

    //set the source portfolio
    trade->set_source_portfolio(source_portfolio);

    // insert the new trade
    this->trades.insert({trade_id,trade});
    this->trade_counter = 1;
};

Position::Position(shared_ptr<Order> filled_order_, unsigned int trade_id_,  Portfolio* source_portfolio )
{   
    //populate common position values
    this->asset_id = filled_order_->get_asset_id();
    this->exchange_id = filled_order_->get_exchange_id();
    this->units = filled_order_->get_units();
    this->is_open = true;

    // populate order values
    this->average_price = filled_order_->get_average_price();
    this->last_price = filled_order_->get_average_price();
    this->position_open_time = filled_order_->get_fill_time();

    // insert the new trade
    this->trades.insert({trade_id_,
                         std::make_shared<Trade>(
                            filled_order_, 
                            trade_id_
                            )});
    this->trade_counter = 1;
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

shared_ptr<Trade> Position::adjust_trade(trade_sp_t trade){
    auto units_ = trade->get_units();
    auto fill_price = trade->get_average_price();

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

    //insert new trade into trades map
    if(trade->get_is_open()){

        //make sure trade does not already exists
        #ifdef ARGUS_RUNTIME_ASSERT        
        assert(!this->trades.contains(trade->get_trade_id()));
        #endif

        //add the trade to the position's trades map
        this->trades.insert({trade->get_trade_id(), trade});
    }
    //remove existing trade from the trades map
    else{
        this->trades.erase(trade->get_trade_id());
    }
    return trade;
}

shared_ptr<Trade> Position::adjust_order(order_sp_t filled_order, Portfolio* portfolio){
    auto units_ = filled_order->get_units();
    auto fill_price = filled_order->get_average_price();

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
    if (!this->trades.contains(trade_id_uint))
    {   
        // trade id was passed but is not in position's child trade map so create new trade
        this->trades.insert({trade_id_uint,
                            std::make_shared<Trade>(
                                filled_order,
                                filled_order->get_trade_id()
                                )});
        this->trade_counter++;
        return this->trades.at(this->trade_counter - 1);
    }
    else
    {   
        // found the trade currently open
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

void Position::generate_order_inverse(std::vector<order_sp_t>& orders){
    for(auto &trade_pair : this->trades){
        orders.push_back(trade_pair.second->generate_order_inverse());
    }
}
