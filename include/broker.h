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

class Broker;

typedef tsl::robin_map<string, shared_ptr<Broker>> Brokers;
typedef shared_ptr<Broker> broker_sp_t;

class Broker
{

public:
    using portfolio_sp_threaded_t = Portfolio::portfolio_sp_threaded_t;
    using position_sp_t = Position::position_sp_t;
    using exchanges_sp_t = ExchangeMap::exchanges_sp_t;
    using trade_sp_t = Trade::trade_sp_t;
    using order_sp_t = Order::order_sp_t;

    /// @brief 
    /// @param broker_id        unique id of the broker
    /// @param cash             cash held at broker
    /// @param logging          logging level
    /// @param history          sp to history obj 
    /// @param master_portfolio sp to master potfolio
    Broker(
        string broker_id,
        double cash,
        int logging,
        shared_ptr<History> history,
        shared_ptr<Portfolio> master_portfolio);

    /// build the broker, set member pointers
    /// \param exchanges    container for master exchange map
    void build(exchanges_sp_t exchange_map);

    /**
     * @brief reset broker to it's original state at the start of the simulation
     */
    void reset_broker();

   /**
    * @brief cancel an order by a given order id
    * 
    * @param order_id unique id of the order to cancel
    */
    void cancel_order(unsigned int order_id);

    /**
     * @brief send orders in the open order buffer to their corresponding exchange
     * used to send and process orders that were placed with lazy execution method
     */
    void send_orders();

    /**
     * @brief process a order that has been filled
     * 
     * @param filled_order sp to a filled order object
     */
    void process_filled_order(shared_ptr<Order> filled_order);

    /**
     * @brief process all open orders and look for new fills to process, when we find one
     * we update the corresponding portfolio the order was placed for
     */
    void process_orders();

    /**
     * @brief place a new order to the exchanges 
     * 
     * @param order sp to a new order object
     * @param process_fill wether or not to process the order once it has been filled
     */
    void place_order(shared_ptr<Order> order, bool process_fill = true);

    /**
     * @brief place a new order into the order buffer to be executed at the end of a timestemp
     * 
     * @param order sp to a new order object to place with lazy exectuion
     */
    void place_order_buffer(shared_ptr<Order> order);

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

    /// starting cash held at the broker
    double starting_cash;

    /// open orders held at the broker
    vector<order_sp_t> open_orders;

    /// open orders held at the broker that have not been sent
    vector<order_sp_t> open_orders_buffer;

    /// pointer to exchange map for routing incoming orders
    exchanges_sp_t exchange_map;

    /// broker's account
    Account broker_account;

    /// master portfolio
    portfolio_sp_threaded_t master_portfolio;

    /// smart pointer to historical values container
    shared_ptr<History> history;
};
#endif // ARGUS_BROKER_H
