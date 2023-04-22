//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_BROKER_H
#define ARGUS_BROKER_H
#include <memory>
#include <string>
#include <tsl/robin_map.h>

#include "position.h"
#include "order.h"
#include "account.h"
#include "exchange.h"

using namespace std;

class Broker{
private:
    ///logging level
    int logging;

    ///unique id of the broker
    string broker_id;

    ///order counter
    unsigned int order_counter;

    ///positions counter
    unsigned int position_counter;

    ///cash held at the broker
    double cash;

    ///dummy portfolio containing positions held at the broker
    tsl::robin_map<string,shared_ptr<Position>> portfolio;

    ///open orders held at the broker
    vector<shared_ptr<Order>> open_orders;

    ///open orders held at the broker that have not been sent
    vector<shared_ptr<Order>> open_orders_buffer;

    ///container for historical orders place to the broker
    vector<shared_ptr<Order>> historical_orders;

public:
    /// constructor for the broker class
    /// \param broker_id unique id of the broker
    /// \param cash      amount of cash held by the broker
    /// \param logging   logging level of the broker
    Broker(string broker_id, double cash, int logging);

    /// cancel an existing order by order's unique id
    /// \param order_id id of the order to cancel
    /// \return         smart pointer to the order that has been canceled
    shared_ptr<Order> cancel_order(unsigned int order_id);

    /// send orders in the open order buffer to their corresponding exchange
    /// \param exchanges reference to hashmap containing all the possible exchanges
    void send_orders(tsl::robin_map<string,shared_ptr<Exchange>>& exchanges);

    /// process a filled order that contains an asset id not in the portfolio
    /// \param filled_order reference to smart pointer containing a filled order
    /// \return smart pointer to a new trade
    shared_ptr<Trade> open_position(shared_ptr<Order>&filled_order);

    ///  process a filled order that contains an asset id already exists in the portfolio
    /// \param filled_order reference to smart pointer containing a filled order
    /// \return smart pointer to the trade that the order modified
    shared_ptr<Trade> modify_position(shared_ptr<Order>&filled_order);

    /// process a filled order who's units is the additive inverse of existing positions units
    /// \param filled_order reference to smart pointer containing a filled order
    void close_position(shared_ptr<Order>&filled_order);

    /// process all open orders to check for fills
    /// \param accounts  reference to hashmap containing accounts so we can update on order fills
    /// \param portfolio reference to hashmap containing all positions to update on order fills
    void process_orders(
            tsl::robin_map<string, Account>& accounts
    );

    ///order placement wrappers exposed to python
    void place_market_order(const string& asset_id, double units,
                            const string& exchange_id,
                            const string& account_id,
                            const string& strategy_id);
    void place_limit_order(const string& asset_id, double units, double limit,
                            const string& exchange_id,
                            const string& account_id,
                            const string& strategy_id);

    //void place_limit_order();
    //void place_stop_loss_order();
    //void place_take_profit_order();

    ///
    /// \param filled_order filled order to log
    void log_order_fill(shared_ptr<Order>& filled_order);

};

#endif //ARGUS_BROKER_H
