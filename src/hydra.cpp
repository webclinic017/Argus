//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <memory>
#include <fmt/core.h>



#include "asset.h"
#include "exchange.h"
#include "hydra.h"
#include "order.h"
#include "settings.h"
#include "utils_time.h"

namespace py = pybind11;
using namespace std;

using portfolio_sp_t = Portfolio::portfolio_sp_t;

Hydra::Hydra(int logging_)
{   
    this->logging = logging_;
    this->history = std::make_shared<History>();
    this->brokers = std::make_shared<Brokers>();
    this->exchanges = std::make_shared<Exchanges>();

    this->master_portfolio = std::make_shared<Portfolio>(
            logging_, 
            0,
            "master", 
            this->history,
            nullptr,
            this->brokers,
            this->exchanges);
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

#ifdef ARGUS_STRIP
void Hydra::log(const string& msg){
    if(this->logging == 1){
            auto datetime_str = nanosecond_epoch_time_to_string(this->hydra_time);
            fmt::print("{}:  HYDRA: {}\n", 
                datetime_str,
                msg
            );
    }
}
#endif

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
    if (this->exchanges->empty())
    {
        throw std::runtime_error("no exchanges to build");
    }

    this->datetime_index = new long long[0];
    this->datetime_index_length = 0;

    // build the exchanges
    for (auto it = this->exchanges->begin(); it != this->exchanges->end(); ++it)
    {
        it.value()->build();
    }

    // build the brokers
    for (auto it = this->brokers->begin(); it != this->brokers->end(); ++it)
    {
        it.value()->build(this->exchanges);
    }

    // build the combined datetime index from all the exchanges
    auto datetime_index_ = container_sorted_union(
        *this->exchanges,
        [](const shared_ptr<Exchange> &obj)
        { return obj->get_datetime_index(); },
        [](const shared_ptr<Exchange> &obj)
        { return obj->get_rows(); });

    this->datetime_index = get<0>(datetime_index_);
    this->datetime_index_length = get<1>(datetime_index_);

    //build portfolios with given size
    this->master_portfolio->build(this->datetime_index_length);

    this->is_built = true;
};

portfolio_sp_t Hydra::get_portfolio(const string& portfolio_id){
    if(portfolio_id == this->master_portfolio->get_portfolio_id()){
        return this->master_portfolio;
    }
    else{
        return this->master_portfolio->find_portfolio(portfolio_id);
    }
}

portfolio_sp_t Hydra::new_portfolio(const string & portfolio_id_, double cash_){
    //build new smart pointer to portfolio 
    auto portfolio = std::make_shared<Portfolio>(
        this->logging, 
        cash_, 
        portfolio_id_,
        this->history,
        this->master_portfolio.get(),
        this->brokers,
        this->exchanges
    );

    //add it to the master portfolio
    this->master_portfolio->add_sub_portfolio(portfolio_id_, portfolio);

    //return sp to new child portfolio
    return std::move(portfolio);
}

shared_ptr<Exchange> Hydra::new_exchange(const string &exchange_id)
{
    if (this->exchanges->contains(exchange_id))
    {
        ARGUS_RUNTIME_ERROR("exchange already exists");
    }
    
    // build new exchange wrapped in shared pointer
    auto exchange = make_shared<Exchange>(exchange_id, this->logging);

    // insert a clone of the smart pointer into the exchange
    this->exchanges->emplace(exchange_id, exchange);

    #ifdef ARGUS_STRIP
    if (this->logging == 1)
    {
        fmt::print("HYDRA: NEW EXCHANGE: {} AT {} \n", exchange_id, static_cast<void *>(exchange.get()));
    }
    #endif

    #ifdef DEBUGGING
        printf("new exchange %s: exchange is built at: %p\n", exchange_id.c_str(), exchange.get());
    #endif

    // return the shared pointer to calling function, lifetime of exchange is now linked to they hydra class
    return exchange;
}

shared_ptr<Broker> Hydra::new_broker(const std::string &broker_id, double cash)
{
    if (this->brokers->contains(broker_id))
    {
        throw std::runtime_error("broker already exists");
    }

    // build new broker wrapped in shared pointer
    auto broker = make_shared<Broker>(
        broker_id,
        cash,
        this->logging,
        this->history,
        this->master_portfolio);

    // insert a clone of the smart pointer into the exchange
    this->brokers->emplace(broker_id, broker);

    #ifdef ARGUS_STRIP
    if (this->logging == 1)
    {
        fmt::print("HYDRA: NEW BROKER: {} AT {}\n", broker_id, static_cast<void *>(broker.get()));
    }
    #endif

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
        return this->exchanges->at(exchange_id);
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
        return this->brokers->at(broker_id);
    }
    catch (const std::out_of_range &e)
    {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

void Hydra::cleanup_asset(const string& asset_id){
    //test to see if position exists with that asset id
    auto position = this->master_portfolio->get_position(asset_id);


    if(position.has_value()){
        #ifdef ARGUS_STRIP
        this->log(fmt::format("found expiring position: {}, units: {}", 
            position.value()->get_asset_id(),
            position.value()->get_units()));
        #endif
    
        //generate and send orders needed to close the position
        auto orders_nullopt = this->master_portfolio->generate_order_inverse(asset_id, false, true);
    }

    #ifdef ARGUS_RUNTIME_ASSERT
    assert(!this->master_portfolio->position_exists(asset_id));
    #endif
};

void Hydra::forward_pass()
{
    //current global simulation time
    this->hydra_time = this->datetime_index[this->current_index];

    #ifdef ARGUS_STRIP
    this->log("executing forward pass...");
    #endif

    // build market views for exchanges
    for (auto &exchange_pair : *this->exchanges)
    {
        auto exchange = exchange_pair.second;
        // set the exchange is_close
        exchange->set_on_close(false);

        // if the exchange time is equal to the hydra time then build market view
        if (exchange->get_datetime() == this->hydra_time)
        {
            auto exchange_steaming = exchange->get_market_view();
        }

        // allow exchanges to process open orders
        exchange_pair.second->process_orders();
    }  
    #ifdef ARGUS_STRIP
    this->log("forward pass complete");
    #endif 
}

//ALLOW STRATEGIES TO PLACE ORDERS AT OPEN

void Hydra::on_open(){
     // allow broker to process orders that have been filled or orders that were placed by 
     // strategies with lazy execution
    for (auto &broker_pair : *this->brokers)
    {   
        #ifdef ARGUS_STRIP
        this->log(
          fmt::format("BROKER: {} sending orders...", 
                broker_pair.first
            )
        );
        #endif

        broker_pair.second->send_orders();

        #ifdef ARGUS_STRIP
        this->log(
          fmt::format("BROKER: {} processing orders...", 
                broker_pair.first
            )
        );
        #endif
    
        broker_pair.second->process_orders();
    }   

    #ifdef ARGUS_STRIP
    this->log("evaluating master portfolio...");
    #endif
    // evaluate the master portfolio at the open
    this->master_portfolio->evaluate(false);
    #ifdef ARGUS_STRIP
    this->log("master portfolio evaluation complete");
    #endif

    // move exchanges to close
    for (auto &exchange_pair : *this->exchanges)
    {
        exchange_pair.second->set_on_close(true);
    }
}

//ALLOW STRATEGIES TO PLACE ORDERS AT CLOSE

bool Hydra::backward_pass(){
    #ifdef ARGUS_STRIP
    this->log("executing backward pass...");
    #endif
    // allow exchanges to process orders placed at close
    for (auto &exchange_pair : *this->exchanges)
    {
        exchange_pair.second->process_orders();
    }

    // process any filled orders, or send orders placed at close with lazy execution
    for (auto &broker_pair : *this->brokers)
    {
        broker_pair.second->process_orders();
    }

    // evaluate the master portfolio at the close
    this->master_portfolio->evaluate(true);

    // hanndle any assets done streaming
    for (auto &exchange_pair : *this->exchanges)
    {
        //move exchange to the open
        exchange_pair.second->set_on_close(false);

        //find expired assets and remove them from portfolio and appropriate exchange
        auto expired_assets =  exchange_pair.second->get_expired_assets();
    
        if(!expired_assets.has_value()){
            continue;
        }
        else{
            // close any open positions in the asset
            for(auto const & asset : *expired_assets.value()){
                this->cleanup_asset(asset->get_asset_id());
            }

            //remove the asset from the market and market view
            exchange_pair.second->move_expired_assets();
        }
    }

    // check to see if we have reached the end
    if (this->current_index == datetime_index_length - 1)
    {
        return false;
    }
    else{
        // increment the hydra's current index
        this->current_index++;
    }
    #ifdef ARGUS_STRIP
    this->log("backward pass complete");
    #endif
    return true;
}