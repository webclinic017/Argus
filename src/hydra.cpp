//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <iostream>
#include <string>
#include <memory>
#include <fmt/core.h>

#include "asset.h"
#include "exchange.h"
#include "hydra.h"
#include "settings.h"

namespace py = pybind11;
using namespace std;

Hydra::Hydra(int logging_)
{   
    this->logging = logging_;
    this->history = std::make_shared<History>();
    this->master_portfolio = std::make_shared<Portfolio>(logging_, 0, "master");
}

Hydra::~Hydra()
{
#ifdef DEBUGGING
    printf("MEMORY:   deallocating hydra at : %p \n", this);
#endif
    if (this->is_built)
    {
        delete[] this->datetime_index;
    }
}

shared_ptr<Hydra> new_hydra(int logging_)
{
    auto hydra = make_shared<Hydra>(logging_);
#ifdef DEBUGGING
    printf("MEMORY: allocating new hydra at : %p \n", hydra.get());
#endif
    return hydra;
}

void Hydra::build()
{
    // check to see if the exchange has been built before
    if (this->is_built)
    {
        delete[] this->datetime_index;
    }
    if (this->exchanges.empty())
    {
        throw std::runtime_error("no exchanges to build");
    }

    this->datetime_index = new long long[0];
    this->datetime_index_length = 0;

    // build the exchanges
    for (auto it = this->exchanges.begin(); it != this->exchanges.end(); ++it)
    {
        it.value()->build();
    }

    // build the brokers
    for (auto it = this->brokers.begin(); it != this->brokers.end(); ++it)
    {
        it.value()->build(
            &this->exchanges);
    }

    // build the combined datetime index from all the exchanges
    auto datetime_index_ = container_sorted_union(
        this->exchanges,
        [](const shared_ptr<Exchange> &obj)
        { return obj->get_datetime_index(); },
        [](const shared_ptr<Exchange> &obj)
        { return obj->get_rows(); });
    this->datetime_index = get<0>(datetime_index_);
    this->datetime_index_length = get<1>(datetime_index_);
    this->is_built = true;
};

shared_ptr<Exchange> Hydra::new_exchange(const string &exchange_id)
{
    if (this->exchanges.contains(exchange_id))
    {
        throw std::runtime_error("exchange already exists");
    }

    // build new exchange wrapped in shared pointer
    auto exchange = make_shared<Exchange>(exchange_id, this->logging);

    // insert a clone of the smart pointer into the exchange
    this->exchanges.emplace(exchange_id, exchange);

    if (this->logging == 1)
    {
        fmt::print("HYDRA: NEW EXCHANGE: {} AT {} \n", exchange_id, static_cast<void *>(exchange.get()));
    }

#ifdef DEBUGGING
    printf("new exchange %s: exchange is built at: %p\n", exchange_id.c_str(), exchange.get());
#endif

    // return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return exchange;
}

shared_ptr<Broker> Hydra::new_broker(const std::string &broker_id, double cash)
{
    if (this->exchanges.contains(broker_id))
    {
        throw std::runtime_error("exchange already exists");
    }

    // build new broker wrapped in shared pointer
    auto broker = make_shared<Broker>(
        broker_id,
        cash,
        this->logging,
        this->history,
        this->master_portfolio);

    // insert a clone of the smart pointer into the exchange
    this->brokers.emplace(broker_id, broker);

    if (this->logging == 1)
    {
        fmt::print("HYDRA: NEW BROKER: {} AT {}", broker_id, static_cast<void *>(broker.get()));
    }

#ifdef DEBUGGING
    printf("new exchange %s: exchange is built at: %p\n", exchange_id.c_str(), exchange.get());
#endif

    // return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return broker;
}

shared_ptr<Exchange> Hydra::get_exchange(const std::string &exchange_id)
{
    try
    {
        return this->exchanges.at(exchange_id);
    }
    catch (const std::out_of_range &e)
    {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Broker> Hydra::get_broker(const std::string &broker_id)
{
    try
    {
        return this->brokers.at(broker_id);
    }
    catch (const std::out_of_range &e)
    {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

bool Hydra::forward_pass()
{
    // check to see if we have reached the end
    if (this->current_index == datetime_index_length)
    {
        return false;
    }

    auto hydra_time = this->datetime_index[this->current_index];

    // build market views for exchanges
    for (auto &exchange_pair : this->exchanges)
    {
        auto exchange = exchange_pair.second;

        // set the exchange is_close
        exchange->set_on_close(false);

        // if the exchange time is equal to the hydra time then build market view
        if (exchange->get_datetime() == hydra_time)
        {
            auto exchange_steaming = exchange->get_market_view();
        }
    }

    // increment the hydra's current index
    this->current_index++;

    // allow exchanges to process open orders
    for (auto &exchange_pair : this->exchanges)
    {
        exchange_pair.second->process_orders();
    }
    return true;
}


void Hydra::evaluate_orders_on_open(){
     // allow broker to process orders that have been filled
    for (auto &broker_pair : this->brokers)
    {
        broker_pair.second->process_orders();
    }

    // evaluate the master portfolio at the open
    this->master_portfolio->evaluate(false, false);

    // move the test to the closing period
    for (auto &exchange_pair : this->exchanges)
    {
        auto exchange = exchange_pair.second;
        // set the exchange is_close
        exchange->set_on_close(false);
    }
}

void Hydra::backward_pass(){
    // allow exchanges to process orders placed at close
    for (auto &exchange_pair : this->exchanges)
    {
        exchange_pair.second->process_orders();
    }

    // process any filled orders
    for (auto &broker_pair : this->brokers)
    {
        broker_pair.second->process_orders();
    }

    // evaluate the master portfolio at the close
    this->master_portfolio->evaluate(false, false);
    
}