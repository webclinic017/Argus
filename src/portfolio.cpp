#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <optional>
#include <fmt/core.h>
#include <vector>

#include "order.h"
#include "portfolio.h"
#include "broker.h"
#include "position.h"
#include "settings.h"
#include "utils_time.h"

using portfolio_sp_t = Portfolio::portfolio_sp_t;
using position_sp_t = Position::position_sp_t;
using order_sp_t = Order::order_sp_t;

using namespace std;

unsigned int Portfolio::trade_counter = 0;

Portfolio::Portfolio(
    int logging_, 
    double cash_, 
    string id_, 
    portfolio_sp_t parent_portfolio_) : positions_map()
{
    this->parent_portfolio = parent_portfolio_;
    this->logging = logging_;
    this->cash = cash_;
    this->portfolio_id = std::move(id_);
    this->position_counter = 0;
}

std::optional<position_sp_t> Portfolio::get_position(const string &asset_id)
{
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() == iter)
    {
        return std::nullopt;
    }
    return iter->second;
}

void Portfolio::delete_position(const string &asset_id)
{   
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() == iter)
    {
        throw std::runtime_error("Portfolio::delete_position position does not exist");
    };

    #ifdef ARGUS_RUNTIME_ASSERT
    auto position = this->get_position(asset_id).value();
    auto trade_count = position->get_trade_count();
    assert(trade_count == 0);
    #endif 

    this->positions_map.erase(asset_id);
}

void Portfolio::add_position(const string &asset_id, Portfolio::position_sp_t position)
{
    auto iter = this->positions_map.find(asset_id);
    if (this->positions_map.end() != iter)
    {
        throw std::runtime_error("Portfolio::add_position position already exists");
    }
    this->positions_map.insert({asset_id, position});
}

void Portfolio::on_order_fill(order_sp_t filled_order)
{
    // log the order if needed
    if (this->logging == 1)
    {
        this->log_order_fill(filled_order);
    }

    // no position exists in the portfolio with the filled order's asset_id
    if (!this->position_exists(filled_order->get_asset_id()))
    {
        this->open_position(filled_order);
    }
    else
    {
        auto position = this->get_position(filled_order->get_asset_id()).value();
        // filled order is not closing existing position
        if (position->get_units() + filled_order->get_units() > 1e-7)
        {
            this->modify_position(filled_order);
        }

        // filled order is closing an existing position
        else
        {
            // cancel all open orders linked to position's child trades
            this->position_cancel_order(position);

            /// close the position
            this->close_position(filled_order);
        }
    }

    // place child orders from the filled order
    for (auto &child_order : filled_order->get_child_orders())
    {
        auto broker = this->broker_map->at(child_order->get_broker_id());
        broker.place_order(child_order);
    }
};

void Portfolio::modify_position(shared_ptr<Order> filled_order)
{
    // get the position and account to modify
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id).value();

    // check to see if a trade id was passed, if not assign new trade id
    if(filled_order->get_trade_id() == 1){
        filled_order->set_trade_id(this->trade_counter);
        this->trade_counter++;
    }

    // adjust position and close out trade if needed
    auto trade = position->adjust_order(filled_order, this);

    //test to see if trade was closed
    if (!trade->get_is_open())
    {
        // cancel any orders linked to the closed traded
        this->trade_cancel_order(trade);

        //propgate trade close up portfolio tree
        if(trade->get_portfolio_id() != this->portfolio_id){
            //get the portfolio source
            auto source_portfolio = trade->get_source_portfolio();            
            
            //make sure we found source
            assert(source_portfolio);   

            //propgate trade close up the portfolio tree
            source_portfolio->propogate_trade_close_up(trade);
        }
        else{
            this->propogate_trade_close_up(trade);
        }

        // remember the trade
        this->history->remember_trade(std::move(trade));
    }
    //new trade
    else if (trade->get_trade_open_time() == filled_order->get_fill_time()){
        this->propogate_trade_open_up(trade);
    }

    // get fill info
    auto order_units = filled_order->get_units();
    auto order_fill_price = filled_order->get_average_price();

    // adjust cash for modifying position
    this->cash -= order_units * order_fill_price;
}

void Portfolio::close_position(shared_ptr<Order> filled_order)
{
#ifdef ARGUS_RUNTIME_ASSERT
    assert(this->units + filled_order->get_units() < 1e-7);
#endif 

    // get the position to close and close it 
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id).value();
    position->close(
        filled_order->get_average_price(), 
        filled_order->get_fill_time());

    // adjust cash held at the broker
    this->cash += filled_order->get_units() * filled_order->get_average_price();

    // close all child trade of the position whose broker is equal to current broker id
    auto trades = position->get_trades();
    for (auto it = trades.begin(); it != trades.end();)
    {
        auto trade = it->second;

        // remove the trade from the position container
        it = trades.erase(it);

        //find the source portfolio of the trade then propgate up trade closing
        if(trade->get_portfolio_id() != this->portfolio_id){
            //search for source portfolio
            auto source_portfolio = trade->get_source_portfolio();

            //make sure we found source
            assert(source_portfolio);   

            //propgate  trade close up the portfolio tree
            source_portfolio->propogate_trade_close_up(trade);
        }
        //this is the source portfolio of the trade
        else{
            this->propogate_trade_close_up(trade);
        }

        // push the trade to history
        this->history->remember_trade(std::move(trade));
    }

    // remove the position from portfolio
    this->delete_position(asset_id);

    // push position to history
    this->history->remember_position(std::move(position));
}

void Portfolio::propogate_trade_close_up(trade_sp_t trade_sp){
    //reached master portfolio
    if(!this->parent_portfolio){
        return;
    }

    #ifdef ARGUS_RUNTIME_ASSERT
    auto parent_position = this->parent_portfolio->get_position(trade_sp->get_asset_id());
    assert(parent_position.has_value());
    #endif

    //get the parent position
    auto position = this->parent_portfolio->get_position(trade_sp->get_asset_id()).value();
    position->adjust_trade(trade_sp);

    //test to see if position should be removed
    if(position->get_trade_count() == 0){
        this->parent_portfolio->delete_position(trade_sp->get_asset_id());
        this->history->remember_position(std::move(position));
    }

    //propgate trade close up portfolio tree
    this->parent_portfolio->propogate_trade_close_up(trade_sp);

}

void Portfolio::propogate_trade_open_up(trade_sp_t trade_sp){
    //reached master portfolio
    if(!this->parent_portfolio){
        return;
    }
    
    //position does not exist in portfolio
    if(!this->parent_portfolio->position_exists(trade_sp->get_asset_id())){
            this->open_position(trade_sp);
    }
    //position already exists, add the new trade
    else{
        auto position = *this->parent_portfolio->get_position(trade_sp->get_asset_id());
        position->adjust_trade(trade_sp);
    }
    
    //recursively proprate trade up up portfolio tree
    this->parent_portfolio->parent_portfolio->propogate_trade_open_up(trade_sp);
};

void Portfolio::add_sub_portfolio(const string &portfolio_id, portfolio_sp_t portfolio)
{
    auto iter = this->portfolio_map.find(portfolio_id);
    if (this->portfolio_map.end() != iter)
    {
        throw std::runtime_error("Portfolio::add_sub_portfolio portfolio already exists");
    }
    this->portfolio_map.insert({portfolio_id, portfolio});
}

std::optional<portfolio_sp_t> Portfolio::get_sub_portfolio(const string &portfolio_id)
{
    auto iter = this->portfolio_map.find(portfolio_id);
    if (this->portfolio_map.end() == iter)
    {
        return std::nullopt;
    }
    return this->portfolio_map.at(portfolio_id);
}

void Portfolio::evaluate(bool on_close, bool recursive)
{
    // evaluate all positions in the current portfolio
    for (auto &position_pair : this->positions_map)
    {
        auto asset_id = position_pair.first;
        auto position = position_pair.second;

        // get the exchange the asset is listed on
        auto exchange_id = position->get_exchange_id();
        auto exchange = this->exchanges_sp->at(exchange_id);
        auto market_price = exchange->get_market_price(asset_id);

        if (market_price != 0)
        {
            position->evaluate(market_price, on_close);
        }
    }

    //only evaluate the portfolio's own positions
    if(!recursive){
        return;
    }
    else{
        // recursively evaluate sub portfolios
        for (auto &portfolio_pair : this->portfolio_map)
        {
            portfolio_pair.second->evaluate(on_close, recursive);
        }
    }
}

void Portfolio::position_cancel_order(Broker::position_sp_t position_sp)
{
    auto trades = position_sp->get_trades();
    for (auto it = trades.begin(); it != trades.end();)
    {
        auto trade = it->second;
        // cancel orders whose parent is the closed trade
        for (auto &order : trade->get_open_orders())
        {
            auto broker = this->broker_map->at(order->get_broker_id());
            broker.cancel_order(order->get_order_id());
        }
    }
}

optional<vector<order_sp_t>> Portfolio::generate_order_inverse( 
        const string & asset_id,
        bool send_orders,
        bool send_collapse)
{
    //get position to close
    auto position = this->get_position(asset_id);

    //make sure we found a position
    if(!position.has_value()){
        throw std::runtime_error("failed to find position");
    }

    //populate inverse orders container
    std::vector<order_sp_t> orders;
    position.value()->generate_order_inverse(orders);

    if(send_collapse){
        //genrate consolidated order
        auto orders_consolidated = OrderConsolidated(orders);

        //send order to broker
        auto broker_id = orders[0]->get_broker_id();
        auto broker = &this->broker_map->at(broker_id);
        auto parent_order = orders_consolidated.get_parent_order();
        broker->place_order(parent_order);

        //make sure the order was filled
        assert(parent_order->get_order_state() == FILLED);
        
        //fill child orders using parent fill
        orders_consolidated.fill_child_orders();
        auto child_orders = orders_consolidated.get_child_orders();

        for(auto child_order : child_orders){
            //process child order as if it had been filled directly
            this->on_order_fill(child_order);

            //remember the child order
            this->history->remember_order(std::move(child_order));
        }
        return std::nullopt;
    }
    if (send_orders){
        for(auto& order : orders){
            auto broker_id = order->get_broker_id();
            auto broker = &this->broker_map->at(broker_id);
            broker->place_order(order);

            //make sure the order was filled
            assert(parent_order->get_order_state() == FILLED);

            //process filled order
            this->on_order_fill(order);

            //remember the order
            this->history->remember_order(std::move(order));


            return std::nullopt;
        }
    }
    return orders;
}

void Portfolio::trade_cancel_order(Broker::trade_sp_t &trade_sp)
{
    for (auto &order : trade_sp->get_open_orders())
    {   
        // get corresponding broker for the order then cancel it
        auto broker = this->broker_map->at(order->get_broker_id());
        broker.cancel_order(order->get_order_id());
    }
}

std::optional<portfolio_sp_t> Portfolio::find_portfolio(const string &portfolio_id_){
    //attempting to find the portfolio function was called on
    if(this->portfolio_id == portfolio_id_){
        throw std::runtime_error("attempting portfolios search on itself");
    }

    //look for the portfolio in the sub portfolios
    if(auto child_portfolio = this->get_sub_portfolio(portfolio_id_)){
        return *child_portfolio;
    }

    //portfolio does not exist in sub portfolios, recurisvely search child portfolios
    for(auto& portfolio_pair : this->portfolio_map){
        //search through child
        if(auto target_portfolio = portfolio_pair.second->find_portfolio(portfolio_id_)){
            return *target_portfolio;
        };
    }
    return std::nullopt;
};

void Portfolio::log_position_open(shared_ptr<Position> &new_position)
{
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  PORTFOLIO {}: POSITION {} AVERAGE PRICE AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               new_position->get_position_id(),
               new_position->get_average_price(),
               new_position->get_asset_id());
}

void Portfolio::log_trade_open(trade_sp_t &new_trade)
{
    auto datetime_str = nanosecond_epoch_time_to_string(new_trade->get_trade_open_time());
    fmt::print("{}:  PORTFOLIO {}: TRADE {} OPENED AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               new_trade->get_trade_id(),
               new_trade->get_average_price(),
               new_trade->get_asset_id());
};


void Portfolio::log_order_fill(order_sp_t &filled_order)
{
    auto datetime_str = nanosecond_epoch_time_to_string(filled_order->get_fill_time());
    fmt::print("{}:  PORTFOLIO {}: ORDER {} FILLED AT {}, ASSET_ID: {}",
               datetime_str,
               this->portfolio_id,
               filled_order->get_order_id(),
               filled_order->get_average_price(),
               filled_order->get_asset_id());
};
