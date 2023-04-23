//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_BROKER_H
#define ARGUS_BROKER_H
#include <memory>
#include <string>
#include <tsl/robin_map.h>

#include "history.h"
#include "position.h"
#include "order.h"
#include "account.h"
#include "exchange.h"

using namespace std;

typedef tsl::robin_map<string,shared_ptr<Position>> Portfolio;
typedef tsl::robin_map<string, Account> Accounts;


class Broker {
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

    ///open orders held at the broker
    vector<shared_ptr<Order>> open_orders;

    ///open orders held at the broker that have not been sent
    vector<shared_ptr<Order>> open_orders_buffer;

    ///smart pointer to historical values container
    shared_ptr<History> history;

public:
    /// constructor for the broker class
    /// \param broker_id unique id of the broker
    /// \param cash      amount of cash held by the broker
    /// \param logging   logging level of the broker
    Broker(string broker_id, double cash, int logging, shared_ptr<History> history);

    /// cancel an existing order by order's unique id
    /// \param order_id id of the order to cancel
    /// \return         smart pointer to the order that has been canceled
    shared_ptr<Order> cancel_order(unsigned int order_id);

    /// send orders in the open order buffer to their corresponding exchange
    /// \param exchanges reference to hashmap containing all the possible exchanges
    void send_orders(tsl::robin_map<string, shared_ptr<Exchange>> &exchanges);

    /// process a filled order that contains an asset id not in the portfolio
    /// \param filled_order reference to smart pointer containing a filled order
    /// \return smart pointer to a new trade
    void open_position(
            Portfolio &portfolio,
            Account &account,
            shared_ptr<Order> &filled_order
            );

    ///  process a filled order that contains an asset id already exists in the portfolio
    /// \param filled_order reference to smart pointer containing a filled order
    /// \return smart pointer to the trade that the order modified
    void modify_position(shared_ptr<Order> &filled_order);

    ///
    /// \param portfolio
    /// \param accounts
    /// \param filled_order
    void close_position(Portfolio &portfolio,
            Accounts &accounts,
            shared_ptr<Order> &filled_order
    );

    /// process all open orders to check for fills
    /// \param portfolio reference to hashmap containing all positions to update on order fills
    /// \param accounts  reference to hashmap containing accounts so we can update on order fills
    void process_orders(
            tsl::robin_map<string, shared_ptr<Position>> &portfolio,
            tsl::robin_map<string, Account> &accounts
    );

    ///order placement wrappers exposed to python
    void place_market_order(const string &asset_id, double units,
                            const string &exchange_id,
                            const string &account_id,
                            const string &strategy_id);

    void place_limit_order(const string &asset_id, double units, double limit,
                           const string &exchange_id,
                           const string &account_id,
                           const string &strategy_id);

    //void place_limit_order();
    //void place_stop_loss_order();
    //void place_take_profit_order();

    /// log order fill
    /// \param filled_order filled order to log
    void log_order_fill(shared_ptr<Order> &filled_order);

    /// log position open
    /// \param new_position new position to log
    void log_position_open(shared_ptr<Position> &new_position);

};
#endif //ARGUS_BROKER_H
