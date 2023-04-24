//
// Created by Nathan Tormaschy on 4/23/23.
//
#include <memory>
#include <string>
#include <optional>
#include <fmt/core.h>

#include "portfolio.h"
#include "broker.h"
#include "position.h"
#include "settings.h"
#include "utils_time.h"

using portfolio_sp_t = Portfolio::portfolio_sp_t;
using position_sp_t = Position::position_sp_t;
using order_sp_t = Order::order_sp_t;

Portfolio::Portfolio(int logging_, double cash_, string id_) : positions_map()
{
    this->logging = logging_;
    this->cash = cash_;
    this->portfolio_id = std::move(id_);
    this->position_counter = 0;
}

std::optional<position_sp_t> Portfolio::get_position(const string &asset_id)
{
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() == iter)
    {
        return std::nullopt;
    }
    return iter->second;
}

void Portfolio::delete_position(const string &asset_id)
{
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() == iter)
    {
        throw std::runtime_error("Portfolio::delete_position position does not exist");
    };
}

void Portfolio::add_position(const string &asset_id, Portfolio::position_sp_t position)
{
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() != iter)
    {
        throw std::runtime_error("Portfolio::add_position position already exists");
    }
    this->positions_map.insert({asset_id, position});
}

void Portfolio::on_order_fill(order_sp_t &filled_order)
{
    // log the order if needed
    if (this->logging == 1)
    {
        this->log_order_fill(filled_order);
    }

    // no position exists in the portfolio with the filled order's asset_id
    if (!this->position_exists(filled_order->get_asset_id()))
    {
        this->open_position(filled_order);
    }
    else
    {
        auto position = this->get_position(filled_order->get_asset_id()).value();
        // filled order is not closing existing position
        if (position->get_units() + filled_order->get_units() > 1e-7)
        {
            this->modify_position(filled_order);
        }

        // filled order is closing an existing position
        else
        {
            // cancel all open orders linked to position's child trades
            this->position_cancel_order(position);

            /// close the position
            this->close_position(filled_order);
        }
    }

    // place child orders from the filled order
    for (auto &child_order : filled_order->get_child_orders())
    {
        auto broker = this->broker_map->at(child_order->get_broker_id());
        broker.place_order(child_order);
    }
};

void Portfolio::open_position(shared_ptr<Order> &filled_order)
{
    // build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(filled_order, this->position_counter);
    this->position_counter++;

    // adjust cash held by broker accordingly
    this->cash -= filled_order->get_units() * filled_order->get_fill_price();

    // insert the new position into the portfolio object
    this->add_position(filled_order->get_asset_id(), position);

    // log the position if needed
    if (this->logging == 1)
    {
        this->log_position_open(position);
    }
}

void Portfolio::modify_position(shared_ptr<Order> &filled_order)
{
    // get the position and account to modify
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id).value();

    // adjust position and close out trade if needed
    auto trade = position->adjust(filled_order);
    if (!trade->get_is_open())
    {
        // cancel any orders linked to the closed traded
        this->trade_cancel_order(trade);

        // remember the trade
        this->history->remember_trade(std::move(trade));
    }

    // get fill info
    auto order_units = filled_order->get_units();
    auto order_fill_price = filled_order->get_fill_price();

    // adjust cash for modifying position
    this->cash -= order_units * order_fill_price;
}

void Portfolio::close_position(shared_ptr<Order> &filled_order)
{
    // get the position to close
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id).value();

    // adjust cash held at the broker
    this->cash += filled_order->get_units() * filled_order->get_fill_price();

    // close all child trade of the position whose broker is equal to current broker id
    auto trades = position->get_trades();
    for (auto it = trades.begin(); it != trades.end();)
    {
        auto trade = it->second;

        // close the child trade
        trade->close(filled_order->get_fill_price(), filled_order->get_fill_time());

        // remove the trade from the position container
        it = trades.erase(it);

        // push the trade to history
        this->history->remember_trade(std::move(trade));
    }

    // remove the position from master portfolio
    this->delete_position(asset_id);

    // push position to history
    this->history->remember_position(std::move(position));
}

void Portfolio::add_sub_portfolio(const string &portfolio_id, portfolio_sp_t portfolio)
{
    auto iter = this->portfolio_map.find(portfolio_id);
    if (this->portfolio_map.end() != iter)
    {
        throw std::runtime_error("Portfolio::add_sub_portfolio portfolio already exists");
    }
    this->portfolio_map.insert({portfolio_id, portfolio});
}

std::optional<portfolio_sp_t> Portfolio::get_sub_portfolio(const string &portfolio_id)
{
    auto iter = this->portfolio_map.find(portfolio_id);
    if (this->portfolio_map.end() == iter)
    {
        return std::nullopt;
    }
    return this->portfolio_map.at(portfolio_id);
}

void Portfolio::evaluate(bool on_close, bool recursive)
{
    // evaluate all positions in the current portfolio
    for (auto &position_pair : this->positions_map)
    {
        auto asset_id = position_pair.first;
        auto position = position_pair.second;

        // get the exchange the asset is listed on
        auto exchange_id = position->get_exchange_id();
        auto exchange = this->exchanges_sp->at(exchange_id);
        auto market_price = exchange->get_market_price(asset_id);

        if (market_price != 0)
        {
            position->evaluate(market_price, on_close);
        }
    }

    //only evaluate the portfolio's own positions
    if(!recursive){
        return;
    }
    else{
        // recursively evaluate sub portfolios
        for (auto &portfolio_pair : this->portfolio_map)
        {
            portfolio_pair.second->evaluate(on_close, true);
        }
    }
}

void Portfolio::position_cancel_order(Broker::position_sp_t &position_sp)
{
    auto trades = position_sp->get_trades();
    for (auto it = trades.begin(); it != trades.end();)
    {
        auto trade = it->second;
        // cancel orders whose parent is the closed trade
        for (auto &order : trade->get_open_orders())
        {
            auto broker = this->broker_map->at(order->get_broker_id());
            broker.cancel_order(order->get_order_id());
        }
    }
}

void Portfolio::trade_cancel_order(Broker::trade_sp_t &trade_sp)
{
    for (auto &order : trade_sp->get_open_orders())
    {   
        // get corresponding broker for the order then cancel it
        auto broker = this->broker_map->at(order->get_broker_id());
        broker.cancel_order(order->get_order_id());
    }
}

std::optional<portfolio_sp_t> Portfolio::find_portfolio(const string &portfolio_id_){
    //attempting to find the portfolio function was called on
    if(this->portfolio_id == portfolio_id_){
        throw std::runtime_error("attempting portfolios search on itself");
    }

    //look for the portfolio in the sub portfolios
    if(auto child_portfolio = this->get_sub_portfolio(portfolio_id_)){
        return *child_portfolio;
    }

    //portfolio does not exist in sub portfolios, recurisvely search child portfolios
    for(auto& portfolio_pair : this->portfolio_map){
        //search through child
        if(auto target_portfolio = portfolio_pair.second->find_portfolio(portfolio_id_)){
            return *target_portfolio;
        };
    }
    return std::nullopt;
};

void Portfolio::log_position_open(shared_ptr<Position> &new_position)
{
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  PORTFOLIO {}: POSITION {} AVERAGE PRICE AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               new_position->get_position_id(),
               new_position->get_average_price(),
               new_position->get_asset_id());
}

void Portfolio::log_order_fill(order_sp_t &filled_order)
{
    auto datetime_str = nanosecond_epoch_time_to_string(filled_order->get_fill_time());
    fmt::print("{}:  PORTFOLIO {}: ORDER {} FILLED AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               filled_order->get_order_id(),
               filled_order->get_fill_price(),
               filled_order->get_asset_id());
};
