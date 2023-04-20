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

using namespace std;

class Hydra{
private:
    ///mapping between exchange id and smart pointer to an exchange
    tsl::robin_map<string,shared_ptr<Exchange>> exchanges;

public:
    ///build new exchange
    shared_ptr<Exchange> new_exchange(string exchange_id);

    ///get shared pointer to an exchange
    shared_ptr<Exchange> get_exchange(string exchange_id);

};

///function for creating a shared pointer to a hydra
shared_ptr<Hydra> new_hydra();

#endif //ARGUS_HYDRA_H
