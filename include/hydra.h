//
// Created by Nathan Tormaschy on 4/19/23.
//

#ifndef ARGUS_HYDRA_H
#define ARGUS_HYDRA_H

#include <string>
#include <memory>
#include <tsl/robin_map.h>

#include "asset.h"
#include "exchange.h"
#include "account.h"
#include "portfolio.h"
#include "history.h"
#include "broker.h"

using namespace std;

class Hydra
{
private:
    using portfolio_sp_t = Portfolio::portfolio_sp_t;
    
    typedef shared_ptr<Brokers> brokers_sp_t;

    /// logging level
    int logging;

    /// is the hydra built
    bool is_built{};

    /// mapping between exchange id and smart pointer to an exchange
    Exchanges exchanges{};

    /// mapping between broker id and smart pointer to a broker
    brokers_sp_t brokers{};

    /// mapping between broker id and portfolio held at broker
    portfolio_sp_t master_portfolio{};

    /// container for remembering historical events and structs
    shared_ptr<History> history;

    /// master datetime index of the combined exchanges
    long long *datetime_index{};

    /// current index of the datetime
    size_t current_index{};

    /// length of datetime index
    size_t datetime_index_length{};

public:
    /// hydra constructor
    explicit Hydra(int logging_);

    /// hydra destructor
    ~Hydra();

    /// build all members
    void build();

    // process orders that were placed at the open
    void evaluate_orders_on_open();

    // forward pass of hydra
    void forward_pass();

    // backward pass of hydra
    bool backward_pass();

    /// evaluate the portfolio at the current market prices
    void evaluate_portfolio(bool on_close);

    portfolio_sp_t get_master_portflio() {return this->master_portfolio;}

    // add a new child portfolio to the master portfolio and return sp to it
    portfolio_sp_t new_portfolio(const string & portfolio_id, double cash);

    /// add a  new exchange
    shared_ptr<Exchange> new_exchange(const string &exchange_id);

    /// add a new broker
    broker_sp_t new_broker(const string &broker_id, double cash);

    /// get shared pointer to an exchange
    shared_ptr<Exchange> get_exchange(const string &exchange_id);

    /// get shared pointer to a broker
    broker_sp_t get_broker(const string &broker_id);

    void cleanup_asset(const string& asset_id);
};

/// function for creating a shared pointer to a hydra
shared_ptr<Hydra> new_hydra(int logging);

#endif // ARGUS_HYDRA_H
