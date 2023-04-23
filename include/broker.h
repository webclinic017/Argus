//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_BROKER_H
#define ARGUS_BROKER_H
#include <memory>
#include <string>
#include <tsl/robin_map.h>

#include "portfolio.h"
#include "history.h"
#include "position.h"
#include "order.h"
#include "account.h"
#include "exchange.h"

using namespace std;

class Broker
{

public:
    using portfolio_sp_t = Portfolio::portfolio_sp_t;
    using position_sp_t = Position::position_sp_t;
    using trade_sp_t = Trade::trade_sp_t;
    using order_sp_t = Order::order_sp_t;

    /// constructor for the broker class
    /// \param broker_id unique id of the broker
    /// \param cash      amount of cash held by the broker
    /// \param logging   logging level of the broker
    Broker(
        string broker_id,
        double cash,
        int logging,
        shared_ptr<History> history,
        shared_ptr<Portfolio> portfolio);

    /// build the broker, set member pointers
    /// \param exchanges    container for master exchange map
    void build(Exchanges *exchanges);

    /// cancel orders by order_id
    void cancel_order(unsigned int order_id);

    /// send orders in the open order buffer to their corresponding exchange
    void send_orders();

    /// process a filled order
    /// \param open_order order that has been filled
    void process_filled_order(shared_ptr<Order> &open_order);

    /// process all open orders
    void process_orders();

    void place_order(shared_ptr<Order> &order);

    /// order placement wrappers exposed to python
    void place_market_order(const string &asset_id, double units,
                            const string &exchange_id,
                            const string &portfolio_id,
                            const string &strategy_id,
                            OrderExecutionType order_execution_type = LAZY);

    void place_limit_order(const string &asset_id, double units, double limit,
                           const string &exchange_id,
                           const string &portfolio_id,
                           const string &strategy_id,
                           OrderExecutionType order_execution_type = LAZY);

    // void place_limit_order();
    // void place_stop_loss_order();
    // void place_take_profit_order();

private:
    /// logging level
    int logging;

    /// unique id of the broker
    string broker_id;

    /// order counter
    unsigned int order_counter;

    /// positions counter
    unsigned int position_counter;

    /// cash held at the broker
    double cash;

    /// open orders held at the broker
    vector<order_sp_t> open_orders;

    /// open orders held at the broker that have not been sent
    vector<order_sp_t> open_orders_buffer;

    /// pointer to exchange map for routing incoming orders
    Exchanges *exchanges;

    /// broker's account
    Account broker_account;

    /// master portfolio
    portfolio_sp_t master_portfolio;

    /// smart pointer to historical values container
    shared_ptr<History> history;
};
#endif // ARGUS_BROKER_H
