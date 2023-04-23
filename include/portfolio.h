//
// Created by Nathan Tormaschy on 4/23/23.
//

#ifndef ARGUS_PORTFOLIO_H
#define ARGUS_PORTFOLIO_H

#include <memory>
#include <string>
#include <tsl/robin_map.h>
#include <fmt/core.h>

#include "order.h"
#include "trade.h"
#include "position.h"
#include "account.h"
#include "history.h"

class Portfolio {
public:
    using position_sp_t = Position::position_sp_t;
    using trade_sp_t = Trade::trade_sp_t;

    /// portfolio constructor
    /// \param logging logging level
    /// \param cash    starting cash of the portfolio
    /// \param id      unique id of the portfolio
    Portfolio(int logging, double cash, string id);

    /// open a new position based on a filled order
    /// \param filled_order ref to a sp to order that has been filled
    void open_position(shared_ptr<Order> &filled_order);

    /// close an existing position based on a filled order
    /// \param filled_order ref to a sp to order that has been filled
    void close_position(shared_ptr<Order> &filled_order);

    /// modify an existing postion based on a filled order
    /// \tparam Func template function that cancels all child orders of a trade
    /// \param filled_order ref to a sp to a filled order
    /// \param func function used to cancel all orders linked to a trade if needed
    template<typename Func>
    void modify_position(shared_ptr<Order> &filled_order, Func trade_cancel_orders);

    /// generate a iterator begin and end for the position map
    /// \return pair of iteratores, begin and end, for postion map
    auto get_iterator() {
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

    /// add new position to the portfolio
    /// \param asset_id asset id of the position to add
    /// \param position new position smart pointer
    void add_position(const string &asset_id, position_sp_t position);

    /// remove a position from the map by asset id
    /// \param asset_id asset id of the position to delete
    void delete_position(const string &asset_id);

private:
    typedef tsl::robin_map<std::string, position_sp_t> positions_map_t;
    ///unique id of the portfolio
    std::string portfolio_id;

    /// logging level
    int logging;

    ///mapping between asset id and position smart pointer
    positions_map_t positions_map;

    ///position counter for position ids
    unsigned int position_counter{};

    /// cash held by the portfolio
    double cash;

    /// pointer to the account map
    Accounts *accounts{};

    // shared pointer to history objects
    shared_ptr<History> history;

    /// helped function to log new position
    /// \param new_position ref to sp of new position
    void log_position_open(position_sp_t &new_position);

};

template<typename Func>
void Portfolio::modify_position(shared_ptr<Order> &filled_order, Func trade_cancel_orders) {
    //get the position and account to modify
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id);
    auto account = &accounts->at(filled_order->get_account_id());

    //adjust position and close out trade if needed
    auto trade = position->adjust(filled_order);
    if(!trade->get_is_open()){

        //remove trade from account's portfolio
        account->close_trade(asset_id);

        //cancel any orders linked to the closed traded
        trade_cancel_orders(trade);

        //remember the trade
        this->history->remember_trade(std::move(trade));
    }

    //adjust cash for increasing position
    auto order_units = filled_order->get_units();
    auto order_fill_price = filled_order->get_fill_price();
    if(order_units * position->get_units() > 0){
        account->add_cash( -1 * order_units * order_fill_price);
        this->cash -= order_units * order_fill_price;
    }
        //adjust cash for reducing position
    else{
        account->add_cash(abs(order_units) * order_fill_price);
        this->cash += abs(order_units) * order_fill_price;
    }
}


#endif //ARGUS_PORTFOLIO_H
