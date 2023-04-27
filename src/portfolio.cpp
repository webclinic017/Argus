#include <cstddef>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <optional>
#include <fmt/core.h>
#include <vector>

#include "history.h"
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

Portfolio::Portfolio(
    int logging_, 
    double cash_, 
    string id_, 
    shared_ptr<History> history,
    Portfolio* parent_portfolio_,
    brokers_sp_t brokers_) : positions_map()
{
    this->brokers = brokers_;
    this->parent_portfolio = parent_portfolio_;
    this->history = history;

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
        ARGUS_RUNTIME_ERROR("Portfolio::delete_position position does not exist");
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
        ARGUS_RUNTIME_ERROR("Portfolio::add_position position already exists");
    }
    this->positions_map.insert({asset_id, position});
}

void Portfolio::place_market_order(const string &asset_id_, double units_,
                                const string &exchange_id_,
                                const string &broker_id_,
                                const string &strategy_id_,
                                OrderExecutionType order_execution_type,
                                int trade_id)
{

    // build new smart pointer to shared order
    auto market_order = make_shared<Order>(MARKET_ORDER,
                                           asset_id_,
                                           units_,
                                           exchange_id_,
                                           broker_id_,
                                           this,
                                           strategy_id_,
                                           trade_id);

    auto broker = this->brokers->at(broker_id_);

    if(this->logging){
        this->log_order_create(market_order);
    }

    if (order_execution_type == EAGER)
    {
        // place order directly and process
        broker->place_order(std::move(market_order));
    }
    else
    {
        // push the order to the buffer that will be processed when the buffer is flushed
        broker->place_order_buffer(market_order);
    }
}

void Portfolio::place_limit_order(const string &asset_id_, double units_, double limit_,
                               const string &exchange_id_,
                               const string &broker_id_,
                               const string &strategy_id_,
                               OrderExecutionType order_execution_type,
                               int trade_id)
{       
    // build new smart pointer to shared order
    auto limit_order = make_shared<Order>(LIMIT_ORDER,
                                          asset_id_,
                                          units_,
                                          exchange_id_,
                                          broker_id_,
                                          this,
                                          strategy_id_,
                                          trade_id);

    // set the limit of the order
    limit_order->set_limit(limit_);

    auto broker = this->brokers->at(broker_id_);


    if (order_execution_type == EAGER)
    {
        // place order directly and process
        broker->place_order(std::move(limit_order));
    }
    else
    {
        // push the order to the buffer that will be processed when the buffer is flushed
        broker->place_order_buffer(limit_order);
    }
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
        this->open_position(filled_order, true);
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
            /// close the position
            this->close_position(filled_order);
        }
    }

    // place child orders from the filled order
    for (auto &child_order : filled_order->get_child_orders())
    {
        auto broker = this->brokers->at(child_order->get_broker_id());
        broker->place_order(child_order);
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
        auto portfolio_source = trade->get_source_portfolio();
        //find the source portfolio of the trade then propgate up trade closing
        if(portfolio_source->get_portfolio_id() != this->portfolio_id){
            //get the portfolio source
            auto source_portfolio = trade->get_source_portfolio();            
            
            //make sure we found source
            assert(source_portfolio);   

            //propgate trade close up the portfolio tree
            source_portfolio->propogate_trade_close_up(trade, true);
        }
        else{
            this->propogate_trade_close_up(trade, true);
        }

        // remember the trade
        this->history->remember_trade(std::move(trade));
    }
    //new trade
    else if (trade->get_trade_open_time() == filled_order->get_fill_time()){
        this->propogate_trade_open_up(trade, true);
    }

    // get fill info
    auto order_units = filled_order->get_units();
    auto order_fill_price = filled_order->get_average_price();

    // adjust cash for modifying position
    this->cash -= order_units * order_fill_price;
}

void Portfolio::close_position(shared_ptr<Order> filled_order)
{
    // get the position to close and close it 
    auto asset_id = filled_order->get_asset_id();
    auto position = this->get_position(asset_id).value();

    #ifdef ARGUS_RUNTIME_ASSERT
    assert(position->get_units() + filled_order->get_units() < 1e-7);
    #endif 

    position->close(
        filled_order->get_average_price(), 
        filled_order->get_fill_time());

    if(this->logging == 1){
        this->log_position_close(position);
    }

    // adjust cash held at the broker
    this->cash += filled_order->get_units() * filled_order->get_average_price();

    // close all child trade of the position whose broker is equal to current broker id
    auto trades = position->get_trades();
    for (auto it = trades.begin(); it != trades.end();)
    {
        auto trade = it->second;

        //cancel any open orders for the trade
        this->trade_cancel_order(trade);

        // remove the trade from the position container
        it = trades.erase(it);
        
        //find the source portfolio of the trade then propgate up trade closing
        auto portfolio_source = trade->get_source_portfolio();
        if(portfolio_source->get_portfolio_id() != this->portfolio_id){
            //get source portfolio for trade
            auto source_portfolio = trade->get_source_portfolio();

            //make sure we found source
            assert(source_portfolio);   

            //propgate  trade close up the portfolio tree
            source_portfolio->propogate_trade_close_up(trade, true);
        }
        //this is the source portfolio of the trade
        else{
            this->propogate_trade_close_up(trade, true);

        }

        // push the trade to history, at this point local trade variable should have use count 1
        // history will validate it when it attempts to push to trade history
        this->history->remember_trade(std::move(trade));
    }

    // remove the position from portfolio
    this->delete_position(asset_id);

    // push position to history
    position->set_is_open(false);
    this->history->remember_position(position);
}

void Portfolio::propogate_trade_close_up(trade_sp_t trade_sp, bool adjust_cash){
    //reached master portfolio
  
    #ifdef ARGUS_RUNTIME_ASSERT
    //auto parent_position = this->get_position(trade_sp->get_asset_id());
    //assert(parent_position.has_value());
    #endif

    //get the parent position
    auto position = this->get_position(trade_sp->get_asset_id()).value();
    position->adjust_trade(trade_sp);

    //test to see if position should be removed
    if(position->get_trade_count() == 0){
        if(this!=trade_sp->get_source_portfolio())
        {
            this->delete_position(trade_sp->get_asset_id());
            this->history->remember_position(std::move(position));
        }
    }

    //propgate trade close up portfolio tree
    if(!this->parent_portfolio)
    {
        return;
    }
    else
    {
        this->parent_portfolio->propogate_trade_close_up(trade_sp, adjust_cash);   
    }
}

void Portfolio::propogate_trade_open_up(trade_sp_t trade_sp, bool adjust_cash){
    //reached master portfolio
    if(!this->parent_portfolio){
        return;
    }

    auto parent = this->parent_portfolio;
    
    //position does not exist in portfolio
    if(!parent->position_exists(trade_sp->get_asset_id()))
    {
            parent->open_position(trade_sp, adjust_cash);
    }
    //position already exists, add the new trade
    else
    {
        auto position = *parent->get_position(trade_sp->get_asset_id());
        //insert the trade into the position
        position->adjust_trade(trade_sp);

        //log the new trade open for the parent
        if(this->logging == 1)
        {
        parent->log_trade_open(trade_sp);
        }
    }
    
    //recursively proprate trade up up portfolio tree
    if(parent->parent_portfolio)
    {
        parent->parent_portfolio->propogate_trade_open_up(trade_sp, adjust_cash);
    }
};

portfolio_sp_t Portfolio::create_sub_portfolio(const string& portfolio_id_, double cash_){
    //create new portfolio
    auto portfolio_ = std::make_shared<Portfolio>(
        this->logging, 
        cash, 
        portfolio_id_,
        this->history,
        this,
        this->brokers
    );
    
    //insert into child portfolio map
    this->portfolio_map.insert({portfolio_id, portfolio_});

    //update parent portfolio's values
    this->cash += cash_;

    //return smart pointer to the new portfolio
    return portfolio_;
}

void Portfolio::add_sub_portfolio(const string &portfolio_id_, portfolio_sp_t portfolio_)
{
    auto iter = this->portfolio_map.find(portfolio_id_);
    if (this->portfolio_map.end() != iter)
    {
        ARGUS_RUNTIME_ERROR("Portfolio::add_sub_portfolio portfolio already exists");
    }
    this->portfolio_map.insert({portfolio_id, portfolio_});

    //make sure the parent portfolio of the passed portfolio is equal to this
    assert(this == portfolio_->get_parent_portfolio());

    //update parent portfolio's values
    this->cash += portfolio_->get_cash();

    //propgate all open positions and trade up the portfolio tree
    //done adjust cash, all parent portfolios simply take on child trades, i.e. NLV increases
    for(const auto& postion_pair : portfolio_->positions_map){
        auto position = postion_pair.second;
        
        //loop over all trades in the position
        for(const auto &trade_pair : position->get_trades()){
            auto trade = trade_pair.second;
            
            //propogate trade up portfolio tree
            portfolio_->propogate_trade_open_up(trade, false);
        }
    }
}

std::optional<portfolio_sp_t> Portfolio::get_sub_portfolio(const string &portfolio_id_)
{
    auto iter = this->portfolio_map.find(portfolio_id_);
    if (this->portfolio_map.end() == iter)
    {
        return std::nullopt;
    }
    return this->portfolio_map.at(portfolio_id_);
}

void Portfolio::evaluate_refresh(){
    this->nlv = this->cash;
    this->unrealized_pl = 0;
     for(auto it = this->positions_map.begin(); it != positions_map.end(); ++it) {
        auto position = it->second;

        this->nlv += position->get_nlv();
        this->unrealized_pl += position->get_nlv();
    }
    for(auto& portfolio_pair : this->portfolio_map){
        portfolio_pair.second->evaluate_refresh();
    }
}

void Portfolio::evaluate(bool on_close)
{
    #ifdef ARGUS_RUNTIME_ASSERT
    //for performance sake only run from master portfolio
    assert(!this->parent_portfolio);
    #endif

    this->nlv = this->cash;
    this->unrealized_pl = 0;
    // evaluate all positions in the current portfolio. Note valuation will propogate down from whichever
    // portfolio it was called on, i.e. all trades in child portfolios will be evaluated already
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

        this->nlv += position->get_nlv();
        this->unrealized_pl += position->get_nlv();
    }

    // recursively call evaluate refresh on all child portfolios. This refreshes values like nlv and 
    // unrealied pl without having to get market price, we took care of that above
    for(auto& portfolio_pair : this->portfolio_map){
        portfolio_pair.second->evaluate_refresh();
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
            auto broker = this->brokers->at(order->get_broker_id());
            broker->cancel_order(order->get_order_id());
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
        ARGUS_RUNTIME_ERROR("failed to find position");
    }

    //populate inverse orders container
    std::vector<order_sp_t> orders;
    position.value()->generate_order_inverse(orders);

    if(send_collapse){
        //genrate consolidated order
        auto orders_consolidated = OrderConsolidated(orders);

        //send order to broker
        auto broker_id = orders[0]->get_broker_id();
        auto broker = this->brokers->at(broker_id);
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
            auto broker = this->brokers->at(broker_id);
            broker->place_order(order);

            //make sure the order was filled
            assert(order->get_order_state() == FILLED);

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
    if(this->logging == 1){
        fmt::print("PORTFOLIO: {} canceling orders for trade: {} \n",
            this->portfolio_id, 
            trade_sp->get_trade_id()
        );
    }
    for (auto &order : trade_sp->get_open_orders())
    {   
        // get corresponding broker for the order then cancel it
        auto broker = this->brokers->at(order->get_broker_id());
        broker->cancel_order(order->get_order_id());
    }
    if(this->logging == 1){
        fmt::print("PORTFOLIO: {} order canceled for trade: {} \n",
            this->portfolio_id, 
            trade_sp->get_trade_id()
        );
    }
}

portfolio_sp_t Portfolio::find_portfolio(const string &portfolio_id_){
    if(this->portfolio_id == portfolio_id_){
        return shared_from_this();
    }
    
    //portfolio does not exist in sub portfolios, recurisvely search child portfolios
    for(auto& portfolio_pair : this->portfolio_map){
        auto portfolio = portfolio_pair.second;
        if (auto found = portfolio->find_portfolio(portfolio_id_)) 
        {
            return found;
        }
    }
    ARGUS_RUNTIME_ERROR("failed to find portfolio");
};

void Portfolio::log_position_open(shared_ptr<Position> &new_position)
{
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  PORTFOLIO {} NEW POSITION: POSITION {} AVERAGE PRICE AT {}, ASSET_ID: {}\n",
               datetime_str,
               this->portfolio_id,
               new_position->get_position_id(),
               new_position->get_average_price(),
               new_position->get_asset_id());
}

void Portfolio::log_position_close(shared_ptr<Position> &new_position)
{
    auto datetime_str = nanosecond_epoch_time_to_string(new_position->get_position_open_time());
    fmt::print("{}:  PORTFOLIO {} CLOSED POSITION: POSITION {} CLOSE PRICE AT {}, ASSET_ID: {}\n",
               datetime_str,
               this->portfolio_id,
               new_position->get_position_id(),
               new_position->get_close_price(),
               new_position->get_asset_id());
}

void Portfolio::log_trade_open(trade_sp_t &new_trade)
{   
    auto datetime_str = nanosecond_epoch_time_to_string(new_trade->get_trade_open_time());
    fmt::print("{}:  PORTFOLIO {} TRADE OPENED: source portfolio id: {}, trade id: {}, asset id: {}, avg price: {}\n",
               datetime_str,
               this->portfolio_id,
               new_trade->get_source_portfolio()->get_portfolio_id(),
               new_trade->get_trade_id(),
               new_trade->get_asset_id(),
               new_trade->get_average_price());
};

void Portfolio::log_order_create(order_sp_t &filled_order)
{
    fmt::print("PORTFOLIO {} ORDER CREATED: order id:  {}, asset id: {}, trade id: {}\n",
               this->portfolio_id,
               filled_order->get_order_id(),
               filled_order->get_asset_id(),
               filled_order->get_trade_id());
};

void Portfolio::log_order_fill(order_sp_t &filled_order)
{
    auto datetime_str = nanosecond_epoch_time_to_string(filled_order->get_fill_time());
    fmt::print("{}:  PORTFOLIO {} ORDER FILLED: order id: {}, asset id: {}, trade id: {}, avg price: {}\n",
                datetime_str,
                this->portfolio_id,
                filled_order->get_order_id(),
                filled_order->get_asset_id(),
                filled_order->get_trade_id(),
                filled_order->get_average_price());
};
