//
// Created by Nathan Tormaschy on 4/19/23.
//
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include <fmt/core.h>
#include <thread>

#include "asset.h"
#include "exchange.h"
#include "history.h"
#include "hydra.h"
#include "order.h"
#include "settings.h"
#include "utils_time.h"

namespace py = pybind11;
using namespace std;

using portfolio_sp_t = Portfolio::portfolio_sp_t;
using exchanges_sp_t = ExchangeMap::exchanges_sp_t; 

Hydra::Hydra(int logging_, double cash_) : master_portfolio(nullptr)
{   
    this->logging = logging_;
    this->history = std::make_shared<History>();
    this->brokers = std::make_shared<Brokers>();
    this->exchange_map = std::make_shared<ExchangeMap>();

    this->master_portfolio = std::make_shared<Portfolio> (
            logging_, 
            cash_,
            "master", 
            this->history,
            nullptr,
            this->brokers,
            this->exchange_map);   
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
    auto datetime_str = nanosecond_epoch_time_to_string(this->hydra_time);
            fmt::print("{}:  HYDRA: {}\n", 
                datetime_str,
                msg
            );
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

void Hydra::reset(bool clear_history, bool clear_strategies)
{
    if(this->logging)
    {
        this->log("reseting hydra");
    }
    this->current_index = 0;
    
    //reset exchanges
    this->exchange_map->reset_exchange_map();

    // reset brokers
    for (auto it = this->brokers->begin(); it != this->brokers->end(); ++it)
    {
        it->second->reset_broker();
    }

    //reset all portfolios
    this->master_portfolio->reset();

    // cleat the history if needed
    if(clear_history)
    {
        auto history = this->history;
        history->reset();
    }
    // remove existing strategies if needed
    if(clear_strategies)
    {
        this->strategies.clear();
    }

    if(this->logging)
    {
        this->log("hydra reset");
    }
}

void Hydra::build()
{
    // check to see if the exchange has been built before
    if (this->is_built)
    {
        delete[] this->datetime_index;
    }
    if (this->exchange_map->exchanges.empty())
    {
        throw std::runtime_error("no exchanges to build");
    }

    this->datetime_index = new long long[0];
    this->datetime_index_length = 0;
    this->candles = 0;

    // build the exchanges
    for (auto it = this->exchange_map->exchanges.begin(); it != this->exchange_map->exchanges.end(); ++it)
    {
        it->second->build();
        this->candles += it->second->candles;

        // build the asset map used to look up asset information
        for(auto& asset_pair : it->second->market){
            this->exchange_map->asset_map[asset_pair.first] = asset_pair.second.get();
        }
    }

    // build the brokers
    for (auto it = this->brokers->begin(); it != this->brokers->end(); ++it)
    {
        it->second->build(this->exchange_map);
    }

    // build the combined datetime index from all the exchanges
    auto datetime_index_ = container_sorted_union(
        this->exchange_map->exchanges,
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

shared_ptr<Portfolio> Hydra::get_portfolio(const string& portfolio_id){
    if(portfolio_id == this->master_portfolio->get_portfolio_id()){
        return this->master_portfolio;
    }
    else{
        return this->master_portfolio->find_portfolio(portfolio_id);
    }
}

shared_ptr<Portfolio> Hydra::new_portfolio(const string & portfolio_id_, double cash_){
    //build new smart pointer to portfolio 
     auto portfolio = std::make_shared<Portfolio>(
        this->logging, 
        cash_, 
        portfolio_id_,
        this->history,
        this->master_portfolio.get(),
        this->brokers,
        this->exchange_map
    );
    //this->master_portfolio
    //need to adjust all portfolios starting cash

    //add it to the master portfolio
    this->master_portfolio->add_sub_portfolio(portfolio_id_, portfolio);

    //return sp to new child portfolio
    return portfolio;
}

shared_ptr<Strategy> Hydra::new_strategy(){
    auto strategy = std::make_shared<Strategy>();
    this->strategies.push_back(strategy);
    return strategy;
}

shared_ptr<Exchange> Hydra::new_exchange(const string &exchange_id)
{
    if (this->exchange_map->exchanges.count(exchange_id))
    {
        ARGUS_RUNTIME_ERROR("exchange already exists");
    }
    
    // build new exchange wrapped in shared pointer
    auto exchange = make_shared<Exchange>(exchange_id, this->logging);

    // insert a clone of the smart pointer into the exchange
    this->exchange_map->exchanges.emplace(exchange_id, exchange);

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
    if (this->brokers->count(broker_id))
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
        return this->exchange_map->exchanges.at(exchange_id);
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

py::array_t<long long> Hydra::get_datetime_index_view()
{
    if (!this->is_built)
    {
        throw std::runtime_error("hydra is not built");
    }
    return to_py_array(
        this->datetime_index,
        this->datetime_index_length,
        true);
}


void Hydra::cleanup_asset(const string& asset_id){
    //test to see if position exists with that asset id
    auto position = this->master_portfolio->get_position(asset_id);


    if(position.has_value()){
        #ifdef ARGUS_STRIP
        if(this->logging == 1)
        {
        this->log(fmt::format("found expiring position: {}, units: {}", 
            position.value()->get_asset_id(),
            position.value()->get_units()));
        }
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
    if(this->logging == 1)
    {
        this->log("executing forward pass...");
    }
    #endif

    // build market views for exchanges
    for (auto &exchange_pair : this->exchange_map->exchanges)
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
    if(this->logging == 1)
    {
        this->log("forward pass complete");
    }
    #endif 
}

void Hydra::on_open(){
     // allow broker to process orders that have been filled or orders that were placed by 
     // strategies with lazy execution
    for (auto &broker_pair : *this->brokers)
    {   
        #ifdef ARGUS_STRIP
        if(this->logging == 1)
        {
            this->log(
            fmt::format("BROKER: {} sending orders...", 
                    broker_pair.first
                )
            );
        }
        #endif

        broker_pair.second->send_orders();

        #ifdef ARGUS_STRIP
        if(this->logging == 1)
        {
            this->log(
            fmt::format("BROKER: {} processing orders...", 
                    broker_pair.first
                )
            );
        }
        #endif
    
        broker_pair.second->process_orders();
    }   


    // move exchanges to close
    for (auto &exchange_pair : this->exchange_map->exchanges)
    {
        exchange_pair.second->set_on_close(true);
    }

    #ifdef ARGUS_STRIP
    if(this->logging == 1)
    {
        this->log("evaluating master portfolio...");
    }
    #endif

    //evaluate master portfolio at close
    this->master_portfolio->evaluate(true);

    #ifdef ARGUS_STRIP
    if(this->logging == 1)
    {
        this->log("master portfolio evaluation complete");
    }
    #endif

}

void Hydra::backward_pass(){
    #ifdef ARGUS_STRIP
    if(this->logging == 1)
    {
        this->log("executing backward pass...");
    }
    #endif

    // send any orders that were placed with lazy execution
    for (auto &broker_pair : *this->brokers)
    {
        broker_pair.second->send_orders();
    }

    // allow exchanges to process orders placed at close
    for (auto &exchange_pair : this->exchange_map->exchanges)
    {
        exchange_pair.second->process_orders();
    }

    // process any orders that have just been filled
    for (auto &broker_pair : *this->brokers)
    {
        broker_pair.second->process_orders();
    }

        
    //update historicals values
    this->master_portfolio->update();

    // hanndle any assets done streaming
    if(this->current_index < datetime_index_length - 1)
    {
        for (auto &exchange_pair : this->exchange_map->exchanges)
        {
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
    }

    // increment the hydra's current index
    this->current_index++;
    
    #ifdef ARGUS_STRIP
    if(this->logging == 1)
    {
        this->log("backward pass complete");
    }
    #endif
}

void Hydra::process_order_history(
    vector<shared_ptr<Order>>& order_history,
    bool on_close, 
    size_t& current_order_index)
{
    auto history_size = order_history.size();
    while(true)
    {
        if(current_order_index == history_size)
        {
            return;
        }
        auto order = order_history[current_order_index];
        if(order->get_order_create_time() == this->hydra_time)
        {
            if(order->get_placed_on_close() == on_close)
            {
                // find the broker the order was placed to
                auto broker= this->brokers->at(order->get_broker_id());
                
                //unfill the order, and replace it at the broker
                order->unfill();
                broker->place_order(order);

                //move forward to next order
                current_order_index++;
            }
            else{
                // found an order that was placed at close of the current time, move forward
                return;
            }
        }
        else{
            //found an order that is not at the current time stamp, move forward
            return;
        }
    }
}

void Hydra::replay()
{
    //reset the hydra to it's original state, but don't clear the history buffer
    this->reset(false, false);

    //build a new smart pointer to histroy object
    auto new_history = std::make_shared<History>();

    //swap it with the full histroy, now the hydra's history is empty
    std::swap(this->history,new_history);   

    //get the old order history ovject to feed to the hydra
    auto order_history = new_history->get_order_history(); 
    
    if(order_history.size() == 0)
    {
        return;
    }
    
    if(this->logging)
    {
        this->log("\033[1;32mstarting hydra replay\033[0m");
    }

    size_t current_order_index = 0;
    //core event loop
    for(int i = 0; i < this->datetime_index_length; i++)
    {
        //generate market view and handle broker,exchange objects on open
        this->forward_pass();

        //allow strategies to place orders at open
        this->process_order_history(order_history, false, current_order_index);

        // process orders on open
        this->on_open();

        //allow strategies to place orders at close
        this->process_order_history(order_history, true, current_order_index);

        //cleanup and move forward in time
        this->backward_pass();
    }
    if(this->logging)
    {
        this->log("hydra replay complete");
    }
;}

void Hydra::goto_datetime(long long datetime)
{
    if(!this->is_built)
    {
        throw std::runtime_error("hydra must be build first");
    }

    // move exchanges forward in time
    for(auto& exchange_pair : this->exchange_map->exchanges)
    {
        exchange_pair.second->goto_datetime(datetime);
    }

    // move the indivual asssets forward in time
    for(auto& asset_pair : this->exchange_map->asset_map)
    {
        asset_pair.second->goto_datetime(datetime);
    }

    // move the hydra index forward in time
    for(int i = 0; i < this->datetime_index_length; i++)
    {
        if(this->datetime_index[i] == datetime)
        {
            this->current_index = i;
            return;
        }
    }
    throw runtime_error("failed to find datetime in hydra index");
}

void Hydra::run(long long to, size_t steps){
    // make sure the hydra was already been built
    if(!this->is_built)
    {
        throw std::runtime_error("hydra not built");
    }
    if(this->logging)
    {
        this->log("\033[1;32mstarting hydra run\033[0m");
    }

    //core event loop
    for(int i = this->current_index; i < this->datetime_index_length; i++)
    {
        //generate market view and handle broker,exchange objects on open
        this->forward_pass();

        //allow strategies to place orders at open
        for(auto & strategy : this->strategies)
        {
            strategy->cxx_handler_on_open();    
        };

        // process orders on open
        this->on_open();

        //allow strategies to place orders at close
        for(auto & strategy : this->strategies)
        {
            strategy->cxx_handler_on_close();    
        };

        //cleanup and move forward in time
        this->backward_pass();

        //check if running to specific point in time
        if(to && this->hydra_time == to)
        {
            return;
        }
        // check to see if the step count has been reached if passed
        if(steps && i == steps)
        {
            return;
        }
    }

    if(this->logging)
    {
        this->log("hydra run complete");
    }
}