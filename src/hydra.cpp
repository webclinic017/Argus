//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include "asset.h"
#include "exchange.h"
#include "hydra.h"
#include "settings.h"

namespace py = pybind11;
using namespace std;

void Hydra::build(){
    //check to see if the exchange has been built before
    if(this->is_built){
        delete [] this->datetime_index;
    }
    if(this->exchanges.empty()){
        throw std::runtime_error("no exchanges to build");
    }

    this->datetime_index = new long long[0];
    this->datetime_index_length = 0;

    //build the exchanges
    for(auto it = this->exchanges.begin(); it != this->exchanges.end(); ++it){
        it.value()->build();
    }

    //build the combined datetime index from all the exchanges
    auto datetime_index_ = container_sorted_union(
            this->exchanges,
            [](const shared_ptr<Exchange> &obj) { return obj->get_datetime_index(); },
            [](const shared_ptr<Exchange> &obj) { return obj->get_rows(); }
    );
    this->datetime_index = get<0>(datetime_index_);
    this->datetime_index_length = get<1>(datetime_index_);
    this->is_built = true;
};

shared_ptr<Exchange> Hydra::new_exchange(const string& exchange_id){
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

shared_ptr<Exchange> Hydra::get_exchange(const std::string& exchange_id) {
    try {
        return this->exchanges.at(exchange_id);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Broker> Hydra::get_broker(const std::string& broker_id) {
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
    if(this->is_built) {
        delete[] this->datetime_index;
    }
}

shared_ptr<Hydra> new_hydra() {
    auto hydra =  make_shared<Hydra>();
#ifdef DEBUGGING
    printf("MEMORY: allocating new hydra at : %p \n", hydra.get());
#endif
    return hydra;
}