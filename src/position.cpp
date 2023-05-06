//
// Created by Nathan Tormaschy on 4/22/23.
//
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include "fmt/core.h"
#include "order.h"
#include "position.h"
#include "settings.h"
#include "trade.h"

using order_sp_t = Order::order_sp_t;
using trade_sp_t = Trade::trade_sp_t;

Position::Position(trade_sp_t trade){
    //populate common position values
    this->position_id = this->positition_counter;
    this->positition_counter++;
    this->asset_id = trade->get_asset_id();
    this->exchange_id = trade->get_exchange_id();
    this->units = trade->get_units();

    // populate order values
    this->is_open = true;
    this->nlv = trade->get_units() * trade->get_average_price();
    this->average_price = trade->get_average_price();
    this->last_price = trade->get_average_price();
    this->position_open_time = trade->get_trade_open_time();

    //set the source portfolio
    trade->set_source_portfolio(trade->get_source_portfolio());

    // insert the new trade
    this->trades.insert({trade->get_trade_id(),trade});
};

Position::Position(shared_ptr<Order> filled_order_)
{   
    //populate common position values
    this->position_id = this->positition_counter;
    this->positition_counter++;
    this->asset_id = filled_order_->get_asset_id();
    this->exchange_id = filled_order_->get_exchange_id();
    this->units = filled_order_->get_units();
    this->is_open = true;

    // populate order values
    this->nlv = filled_order_->get_units() * filled_order_->get_average_price();
    this->average_price = filled_order_->get_average_price();
    this->last_price = filled_order_->get_average_price();
    this->position_open_time = filled_order_->get_fill_time();

    // insert the new trade
    auto trade = std::make_shared<Trade>(
                            filled_order_
                            );
    this->trades.insert({trade->get_trade_id(),trade});
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

    //insert new trade into trades map
    if(trade->get_is_open()){
        // adjust position units
        this->units += units_;

        //make sure trade does not already exists
        #ifdef ARGUS_RUNTIME_ASSERT        
        if(this->trades.count(trade->get_trade_id())){
            ARGUS_RUNTIME_ERROR("trade already exists");
        }        
        #endif

        //add the trade to the position's trades map
        this->trades.insert({trade->get_trade_id(), trade});
    }
    //remove existing trade from the trades map
    else{
        // adjust position units (subtract trade units as trade units reflect the size of the trade that was closed)
        this->units -= units_;
        if(this->trades.count(trade->get_trade_id()))
        {
            this->trades.erase(trade->get_trade_id());
        }
    }

    //if position is closed update values
    if(this->trades.size() == 0)
    {
        this->close_price = trade->get_close_price();
        this->position_close_time = trade->get_trade_close_time();
        this->is_open = false;
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
    if (filled_order->get_trade_id() == -1)
    {   
        // trade id was passed but is not in position's child trade map so create new trade
        auto trade = std::make_shared<Trade>(filled_order);
        this->trades.insert({trade->get_trade_id(),
                            trade});
        return this->trades.at(trade->get_trade_id());
    }
    else
    {  
        // found the trade currently open
        auto trade_iter = this->trades.find(filled_order->get_trade_id());
        if(trade_iter == this->trades.end())
        {
            ARGUS_RUNTIME_ERROR(
                fmt::format("failed to find trade id: {} in trades\n",filled_order->get_trade_id())
            );
        }
        auto trade = trade_iter->second;
        trade->adjust(filled_order);

        // if the adjustment causes the trade to close remove from position child trades
        if (!trade->get_is_open())
        {
            this->trades.erase(trade->get_trade_id());
        }
        return trade;
        
    }
}

std::optional<trade_sp_t> Position::get_trade(unsigned int trade_id)
{
    auto iter = this->trades.find(trade_id);
    if (this->trades.end() == iter)
    {
        return std::nullopt;
    }
    return iter->second;
}

void Position::generate_order_inverse(std::vector<order_sp_t>& orders){
    for(auto &trade_pair : this->trades){
        auto order = trade_pair.second->generate_order_inverse();
        orders.push_back(order);
    }
}
