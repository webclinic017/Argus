//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <iostream>
#include <string>
#include <memory>
#include "asset.h"
#include "exchange.h"
#include "hydra.h"

namespace py = pybind11;
using namespace std;

shared_ptr<Exchange> Hydra::new_exchange(string exchange_id){
    if(this->exchanges.contains(exchange_id)){
        throw std::runtime_error("exchange already exists");
    }

    //build new exchange wrapped in shared pointer
    auto exchange = make_shared<Exchange>(exchange_id);

    //insert a clone of the smart pointer into the exchange
    this->exchanges.emplace(exchange_id, exchange);

    //return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return exchange;
}

shared_ptr<Exchange> Hydra::get_exchange(std::string exchange_id) {
    try {
        return this->exchanges.at(exchange_id);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Hydra> new_hydra() {
    return make_shared<Hydra>();
}