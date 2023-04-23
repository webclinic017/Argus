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

class Hydra{
private:
    ///logging level
    int logging;

    ///is the hydra built
    bool is_built{};

    ///mapping between exchange id and smart pointer to an exchange
    tsl::robin_map<string,shared_ptr<Exchange>> exchanges;

    ///mapping between broker id and smart pointer to a broker
    tsl::robin_map<string, shared_ptr<Broker>> brokers;

    ///mapping between account id and an account
    tsl::robin_map<string, Account> accounts;

    ///mapping between broker id and portfolio held at broker
    tsl::robin_map<string,shared_ptr<Portfolio>> portfolios;

    ///container for remembering historical events and structs
    shared_ptr<History> history;

    ///master datetime index of the combined exchanges
    long long* datetime_index{};

    ///current index of the datetime
    size_t current_index{};

    ///length of datetime index
    size_t datetime_index_length{};

public:
    ///hydra constructor
    explicit Hydra(int logging_);

    ///hydra destructor
    ~Hydra();

    ///build all members
    void build();

    //forward pass of hydra
    bool forward_pass();

    ///evaluate the portfolio at the current market prices
    void evaluate_portfolio(bool on_close);

    ///add a  new exchange
    shared_ptr<Exchange> new_exchange(const string& exchange_id);

    ///add a new broker
    shared_ptr<Broker> new_broker(const string& broker_id, double cash);

    ///get shared pointer to an exchange
    shared_ptr<Exchange> get_exchange(const string& exchange_id);

    ///get shared pointer to a broker
    shared_ptr<Broker> get_broker(const string& broker_id);

};

///function for creating a shared pointer to a hydra
shared_ptr<Hydra> new_hydra(int logging);

#endif //ARGUS_HYDRA_H
