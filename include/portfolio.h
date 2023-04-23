//
// Created by Nathan Tormaschy on 4/23/23.
//

#ifndef ARGUS_PORTFOLIO_H
#define ARGUS_PORTFOLIO_H

#include <memory>
#include <string>
#include <tsl/robin_map.h>
#include <fmt/core.h>

class Broker;

#include "order.h"
#include "trade.h"
#include "position.h"
#include "account.h"
#include "history.h"
#include "exchange.h"

class Portfolio
{
public:
    using position_sp_t = Position::position_sp_t;
    using trade_sp_t = Trade::trade_sp_t;
    using order_sp_t = Order::order_sp_t;

    typedef shared_ptr<Portfolio> portfolio_sp_t;
    typedef tsl::robin_map<std::string, position_sp_t> positions_map_t;
    typedef tsl::robin_map<std::string, portfolio_sp_t> portfolios_map_t;

    /// portfolio constructor
    /// \param logging logging level
    /// \param cash    starting cash of the portfolio
    /// \param id      unique id of the portfolio
    Portfolio(int logging, double cash, string id);

    void on_order_fill(order_sp_t &filled_order);

    /// generate a iterator begin and end for the position map
    /// \return pair of iteratores, begin and end, for postion map
    auto get_iterator()
    {
        return std::make_pair(this->positions_map.begin(), this->positions_map.end());
    }

    /// does the portfolio contain a position with the given asset id
    /// \param asset_id unique id of the asset
    /// \return does the position exist
    [[nodiscard]] bool position_exists(const string &asset_id) const { return this->positions_map.contains(asset_id); };

    /// get smart pointer to existing position
    /// \param asset_id unique ass id of the position
    /// \return smart pointer to the existing position
    position_sp_t get_position(const string &asset_id);

    /// add new sub portfolio to the portfolio
    /// \param portfolio_id portfolio id of the new sub portfolio
    /// \param portfolio smart pointer to sub portfolio
    void add_sub_portfolio(const string &portfolio_id, portfolio_sp_t portfolio);

    /// @brief get smartpointer to a sub portfolio
    /// @param portfolio_id id of the sub portfolio
    /// @return smart pointer to the sub portfolio
    portfolio_sp_t get_sub_portfolio(const string &portfolio_id);

    /// @brief evaluate the portfolio on open or close
    /// @param on_close are we at close of the candle
    void evaluate(bool on_close);

private:
    /// unique id of the portfolio
    std::string portfolio_id;

    /// logging level
    int logging;

    /// mapping between asset id and position smart pointer
    positions_map_t positions_map;

    /// mapping between sub portfolio id and potfolio smart poitner
    portfolios_map_t portfolio_map;

    /// smart pointer to exchanges map
    exchanges_sp_t exchanges_sp;

    /// smart pointer to broker map
    shared_ptr<tsl::robin_map<string, Broker>> broker_map;

    /// position counter for position ids
    unsigned int position_counter;

    /// cash held by the portfolio
    double cash;

    // shared pointer to history objects
    shared_ptr<History> history;

    /// log order fill
    /// \param filled_order filled order to log
    void log_order_fill(order_sp_t &filled_order);

    /// log position open
    /// \param new_position new position to log
    void log_position_open(shared_ptr<Position> &new_position);

    /// add new position to the map by asset id
    /// \param asset_id asset id of the position to add
    /// \param position new position smart pointer
    inline void add_position(const string &asset_id, position_sp_t position);

    /// remove a position from the map by asset id
    /// \param asset_id asset id of the position to delete
    inline void delete_position(const string &asset_id);

    /// modify an existing postion based on a filled order
    /// \param filled_order ref to a sp to a filled order
    void modify_position(order_sp_t &filled_order);

    /// open a new position based on a filled order
    /// \param filled_order ref to a sp to order that has been filled
    void open_position(order_sp_t &filled_order);

    /// close an existing position based on a filled order
    /// \param filled_order ref to a sp to order that has been filled
    void close_position(order_sp_t &filled_order);

    /// @brief cancel all order for child trades in a position
    /// @param position_sp ref to sp of a position
    void position_cancel_order(position_sp_t &position_sp);

    /// @brief cancel all open orders for a trade
    /// @param trade_sp ref to sp of a trade
    void trade_cancel_order(trade_sp_t &trade_sp);
};

#endif // ARGUS_PORTFOLIO_H