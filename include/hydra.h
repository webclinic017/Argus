//
// Created by Nathan Tormaschy on 4/19/23.
//

#ifndef ARGUS_HYDRA_H
#define ARGUS_HYDRA_H

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

#include "asset.h"
#include "exchange.h"
#include "account.h"
#include "portfolio.h"
#include "history.h"
#include "broker.h"
#include "strategy.h"

using namespace std;

class Hydra
{
private:
    using portfolio_sp_threaded_t = Portfolio::portfolio_sp_threaded_t;
    using exchanges_sp_t = ExchangeMap::exchanges_sp_t;
    
    typedef shared_ptr<Brokers> brokers_sp_t;

    /// logging level
    int logging;

    /// is the hydra built
    bool is_built{};

    /// mapping between broker id and portfolio held at broker
    portfolio_sp_threaded_t master_portfolio;

    /// container for remembering historical events and structs
    shared_ptr<History> history;

    long long hydra_time;

    /// master datetime index of the combined exchanges
    long long *datetime_index{};

    /// current index of the datetime
    size_t current_index = 0;

    /// length of datetime index
    size_t datetime_index_length = 0;

    /// total number of rows in the hydra across all exchanges
    size_t candles = 0;

    // function calls on open
    vector<shared_ptr<Strategy>> strategies;

    void log(const string& msg);

public:
    /// hydra constructor
    Hydra(int logging_, double cash = 0.0);

    /// hydra destructor
    ~Hydra();

    /// mapping between exchange id and smart pointer to an exchange
    exchanges_sp_t exchange_map;

    /// mapping between broker id and smart pointer to a broker
    brokers_sp_t brokers{};
    
    /// build all members
    void build();

    /// reset all members
    void reset();

    // process orders that were placed at the open
    void on_open();

    // forward pass of hydra
    void forward_pass();

    // backward pass of hydra
    bool backward_pass();

    // execute simulation
    void run();

    /// evaluate the portfolio at the current market prices
    void evaluate_portfolio(bool on_close);

    /// get sp to master portfolio
    shared_ptr<Portfolio> get_master_portflio() {return this->master_portfolio.get_shared_ptr();}

    /// search for a portfolio in portfolio tree and return it 
    shared_ptr<Portfolio> get_portfolio(const string & portfolio_id);

    // add a new child portfolio to the master portfolio and return sp to it
    shared_ptr<Portfolio> new_portfolio(const string & portfolio_id, double cash);

    /// get shared pointer to an exchange
    shared_ptr<Exchange> get_exchange(const string &exchange_id);

    /// add a  new exchange to hydra class
    shared_ptr<Exchange> new_exchange(const string &exchange_id);

    /// create new strategy class
    shared_ptr<Strategy> new_strategy();

    /// get shared pointer to a broker
    broker_sp_t get_broker(const string &broker_id);

    /// add a new broker
    broker_sp_t new_broker(const string &broker_id, double cash);

    /// total number of rows loaded
    size_t get_candles(){return this->candles;}
    
    /// handle a asset id that has finished streaming (remove from portfolio and exchange)
    void cleanup_asset(const string& asset_id);

    shared_ptr<History> get_history(){return this->history;}

    //cast self to void ptr and return
    void* void_ptr() { return static_cast<void*>(this);};

};

/// function for creating a shared pointer to a hydra
shared_ptr<Hydra> new_hydra(int logging);

#endif // ARGUS_HYDRA_H
