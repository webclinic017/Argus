#ifndef ARGUS_PORTFOLIO_H
#define ARGUS_PORTFOLIO_H

#include <memory>
#include <string>
#include <optional>
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
    /// @param logging logging level
    /// @param cash    starting cash of the portfolio
    /// @param id      unique id of the portfolio
    Portfolio(int logging, double cash, string id, portfolio_sp_t parent_portfolio);

    void on_order_fill(order_sp_t filled_order);

    /// generate a iterator begin and end for the position map
    /// \return pair of iteratores, begin and end, for postion map
    auto get_iterator()
    {
        return std::make_pair(this->positions_map.begin(), this->positions_map.end());
    }

    /// does the portfolio contain a position with the given asset id
    /// @param asset_id unique id of the asset
    /// \return does the position exist
    [[nodiscard]] bool position_exists(const string &asset_id) const { return this->positions_map.contains(asset_id); };

    /// get smart pointer to existing position
    /// @param asset_id unique ass id of the position
    /// \return smart pointer to the existing position
    std::optional<position_sp_t> get_position(const string &asset_id);

    /// add new sub portfolio to the portfolio
    /// @param portfolio_id portfolio id of the new sub portfolio
    /// @param portfolio smart pointer to sub portfolio
    void add_sub_portfolio(const string &portfolio_id, portfolio_sp_t portfolio);

    /// @brief get smartpointer to a sub portfolio
    /// @param portfolio_id id of the sub portfolio
    /// @return smart pointer to the sub portfolio
    std::optional<portfolio_sp_t> get_sub_portfolio(const string &portfolio_id);

    /// @brief recursively search through sub portfolios to find by portfolio id
    /// @param portfolio_id unique id of the portfolio
    /// @return sp to portfolio if exists
    std::optional<portfolio_sp_t> find_portfolio(const string &portfolio_id);

    /// @brief evaluate the portfolio on open or close
    /// @param on_close are we at close of the candle
    /// @param recursive wether to recursievly evalute all portfolios
    void evaluate(bool on_close, bool recursive);

    /// @brief generate and send nessecary orders to completely exist position by asset id (including all child portfolios)
    /// @param orders to vector to hold inverse orders
    std::optional<std::vector<order_sp_t>> generate_order_inverse( 
        const string & asset_id,
        bool send_orders,
        bool send_collapse);

private:
    /// unique id of the portfolio
    std::string portfolio_id;

    /// logging level
    int logging;

    /// smart pointer to parent_portfolio
    portfolio_sp_t parent_portfolio;

    /// mapping between sub portfolio id and potfolio smart poitner
    portfolios_map_t portfolio_map;

    /// mapping between asset id and position smart pointer
    positions_map_t positions_map;

    /// smart pointer to exchanges map
    exchanges_sp_t exchanges_sp;

    /// smart pointer to broker map
    shared_ptr<tsl::robin_map<string, Broker>> broker_map;

    /// position counter for position ids
    unsigned int position_counter;

    static unsigned int trade_counter;

    /// cash held by the portfolio
    double cash;

    // shared pointer to history objects
    shared_ptr<History> history;

    /// @brief add new position to the map by asset id
    /// @param asset_id asset id of the position to add
    /// @param position new position smart pointer
    inline void add_position(const string &asset_id, position_sp_t position);

    /// @brief remove a position from the map by asset id
    /// @param asset_id asset id of the position to delete
    inline void delete_position(const string &asset_id);

    /// @brief modify an existing postion based on a filled order
    /// @param filled_order ref to a sp to a filled order
    void modify_position(order_sp_t filled_order);

    /// @brief open a new position based on either filled order or new trade
    /// @param open_obj sp to either a new trade or a new order
    template<typename T>
    void open_position(T open_obj);

    /// @brief close an existing position based on a filled order
    /// @param filled_order ref to a sp to order that has been filled
    void close_position(order_sp_t filled_order);

    /// @brief cancel all order for child trades in a position
    /// @param position_sp ref to sp of a position
    void position_cancel_order(position_sp_t position_sp);

    /// @brief cancel all open orders for a trade
    /// @param trade_sp ref to sp of a trade
    void trade_cancel_order(trade_sp_t &trade_sp);

    /// @brief propogate a new trade up the portfolio tree
    void propogate_trade_open_up(trade_sp_t trade_sp);

    /// @brief propogate a trade close up the portfolio tree
    void propogate_trade_close_up(trade_sp_t trade_sp);

    /// log order fill
    /// @param filled_order filled order to log
    void log_order_fill(order_sp_t &filled_order);

    /// log position open
    /// @param new_position new position to log
    void log_position_open(shared_ptr<Position> &new_position);

    /// log trade open
    /// @param new_trade new trade to log
    void log_trade_open(trade_sp_t &new_trade);

};

template<typename T>
void Portfolio::open_position(T open_obj)
{   
    // build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(open_obj, this->trade_counter, this);
    position->set_position_id(this->position_counter);
    this->position_counter++;
    this->trade_counter++;

    //propgate the new trade up portfolio tree
    auto trade_sp = position->get_trade(this->trade_counter - 1);
    this->propogate_trade_open_up(trade_sp);

    // adjust cash held by broker accordingly
    this->cash -= open_obj->get_units() * open_obj->get_average_price();

    // insert the new position into the portfolio object
    this->add_position(open_obj->get_asset_id(), position);

    // log the position if needed
    if (this->logging == 1)
    {
        this->log_position_open(position);
        this->log_trade_open(trade_sp);
    }
}

#endif // ARGUS_PORTFOLIO_H