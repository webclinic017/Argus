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
#include "exchange.h"

using namespace std;

class Broker{
private:
    ///logging level
    int logging;

    ///unique id of the broker
    string broker_id;

    ///cash held at the broker
    double cash;

    ///open orders held at the broker
    vector<shared_ptr<Order>> open_orders;

    ///open orders held at the broker that have not been sent
    vector<shared_ptr<Order>> open_orders_buffer;


    ///container for historical orders place to the broker
    vector<shared_ptr<Order>> historical_orders;

public:
    ///broker constructor
    Broker(string broker_id, double cash, int logging);

    ///broker cancel order
    shared_ptr<Order> cancel_order(unsigned int order_id);

    ///send all orders in the buffer to the corresponding exchange
    void send_orders( tsl::robin_map<string,shared_ptr<Exchange>>& exchanges);

    ///process all orders to check for fills
    void process_orders();

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
