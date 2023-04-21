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
#include "broker.h"

using namespace std;

class Hydra{
private:
    ///mapping between exchange id and smart pointer to an exchange
    tsl::robin_map<string,shared_ptr<Exchange>> exchanges;

    ///mapping between broker id and smart pointer to a broker
    tsl::robin_map<string, shared_ptr<Broker>> brokers;

    ///mapping between account id and an account
    tsl::robin_map<string, Account> accounts;

    ///portfolio containing all positions
    tsl::robin_map<string,shared_ptr<Position>> portfolio;

public:
    ///hydra destructor
    ~Hydra();

    ///build all members
    void build();

    ///add a  new exchange
    shared_ptr<Exchange> new_exchange(string exchange_id);

    ///add a new broker
    shared_ptr<Broker> new_broker(const string& broker_id, double cash);

    ///get shared pointer to an exchange
    shared_ptr<Exchange> get_exchange(string exchange_id);

    ///get shared pointer to a broker
    shared_ptr<Broker> get_broker(string broker_id);

};

///function for creating a shared pointer to a hydra
shared_ptr<Hydra> new_hydra();

#endif //ARGUS_HYDRA_H
