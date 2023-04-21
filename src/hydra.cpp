//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <iostream>
#include <string>
#include <memory>
#include "asset.h"
#include "exchange.h"
#include "hydra.h"
#include "settings.h"

namespace py = pybind11;
using namespace std;

void Hydra::build(){};

shared_ptr<Exchange> Hydra::new_exchange(string exchange_id){
    if(this->exchanges.contains(exchange_id)){
        throw std::runtime_error("exchange already exists");
    }

    //build new exchange wrapped in shared pointer
    auto exchange = make_shared<Exchange>(exchange_id);

    //insert a clone of the smart pointer into the exchange
    this->exchanges.emplace(exchange_id, exchange);

#ifdef DEBUGGING
    printf("new exchange %s: exchange is built at: %p\n", exchange_id.c_str(), exchange.get());
#endif

    //return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return exchange;
}

shared_ptr<Broker> Hydra::new_broker(const std::string& broker_id, double cash) {
    if(this->exchanges.contains(broker_id)){
        throw std::runtime_error("exchange already exists");
    }

    //build new exchange wrapped in shared pointer
    auto broker = make_shared<Broker>(broker_id, cash);

    //insert a clone of the smart pointer into the exchange
    this->brokers.emplace(broker_id, broker);

#ifdef DEBUGGING
    printf("new exchange %s: exchange is built at: %p\n", exchange_id.c_str(), exchange.get());
#endif

    //return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return broker;
}

shared_ptr<Exchange> Hydra::get_exchange(std::string exchange_id) {
    try {
        return this->exchanges.at(exchange_id);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Broker> Hydra::get_broker(std::string broker_id) {
    try {
        return this->brokers.at(broker_id);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

Hydra::~Hydra() {
#ifdef DEBUGGING
    printf("MEMORY:   deallocating hydra at : %p \n", this);
#endif
}

shared_ptr<Hydra> new_hydra() {
    auto hydra =  make_shared<Hydra>();
#ifdef DEBUGGING
    printf("MEMORY: allocating new hydra at : %p \n", hydra.get());
#endif
    return hydra;
}