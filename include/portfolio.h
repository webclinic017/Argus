#ifndef ARGUS_PORTFOLIO_H
#define ARGUS_PORTFOLIO_H

#include <memory>
#include <string>
#include <optional>
#include <sys/_types/_size_t.h>
#include <tsl/robin_map.h>
#include <fmt/core.h>
#include <mutex>

class Broker;

#include "order.h"
#include "trade.h"
#include "position.h"
#include "account.h"
#include "exchange.h"
#include "settings.h"

class PortfolioHistory;
class PortfolioTracer;
class EventTracer;

enum PortfolioTracerType
{
    Value,
    Event
};

class Portfolio : public std::enable_shared_from_this<Portfolio>
{
public:

    using position_sp_t = Position::position_sp_t;
    using exchanges_sp_t = ExchangeMap::exchanges_sp_t;
    using trade_sp_t = Trade::trade_sp_t;
    using order_sp_t = Order::order_sp_t;

    typedef shared_ptr<Portfolio> portfolio_sp_t;
    typedef tsl::robin_map<std::string, position_sp_t> positions_map_t;
    typedef tsl::robin_map<std::string, portfolio_sp_t> portfolios_map_t;
    typedef shared_ptr<tsl::robin_map<string, shared_ptr<Broker>>> brokers_sp_t;

    /// portfolio constructor
    /// @param logging logging level
    /// @param cash    starting cash of the portfolio
    /// @param id      unique id of the portfolio
    Portfolio(
        int logging, 
        double cash, 
        string id, 
        Portfolio* parent_portfolio_,
        brokers_sp_t brokers_,
        exchanges_sp_t exchanges_
        );

    /**
     * @brief build the portfolio object with a given evaluation size
     * 
     * @param portfolio_eval_length amount of memory to allocate to tracking values across the simulation
     */
    void build(size_t portfolio_eval_length);

    /**
     * @brief reset a portfolio object to its original state
     * 
     */
    void reset(bool clear_history = true);

    /// get the memory addres of the portfolio object
    auto get_mem_address(){return reinterpret_cast<std::uintptr_t>(this); }
    
    /// @brief the amount of cash held by the portfolio (recursive sum of all child portfolios)
    double get_cash() const {return to_double(this->cash);}

    /// @brief get the net liquidation as last calculated
    /// @return the net liquidation value of the portfolio
    double get_nlv() const {return to_double(this->nlv);}

    /// @brief get the net liquidation as last calculated
    double get_unrealized_pl() const {return this->unrealized_pl;}
    
    /// @brief function to handle a order fill event
    /// @param filled_order a sp to a new filled order recieved from a broker
    void on_order_fill(order_sp_t filled_order);

    /// does the portfolio contain a position with the given asset id
    /// @param asset_id unique id of the asset
    /// \return does the position exist
    [[nodiscard]] bool position_exists(const string &asset_id) const { return this->positions_map.count(asset_id); };

    /// @brief get sp to portfolio history object
    shared_ptr<PortfolioHistory> get_portfolio_history(){return this->portfolio_history;};
    
    /// get smart pointer to existing position
    /// @param asset_id unique ass id of the position
    /// \return smart pointer to the existing position
    std::optional<position_sp_t> get_position(const string &asset_id);

    /// get the parent portfolio pointer
    Portfolio* get_parent_portfolio(){return this->parent_portfolio;}

    /// add new sub portfolio to the portfolio
    /// @param portfolio_id portfolio id of the new sub portfolio
    /// @param portfolio smart pointer to sub portfolio
    void add_sub_portfolio(const string &portfolio_id, portfolio_sp_t portfolio);
    
    /// create a new sub portfolio to the parent
    /// @param portfolio_id portfolio id of the new sub portfolio
    /// @param amount of cash held in the new portfolio
    shared_ptr<Portfolio> create_sub_portfolio(const string &portfolio_id, double cash);

    /// @brief get smartpointer to a sub portfolio
    /// @param portfolio_id id of the sub portfolio
    /// @return smart pointer to the sub portfolio
    optional<portfolio_sp_t>  get_sub_portfolio(const string &portfolio_id);

    /// @brief recursively search through sub portfolios to find by portfolio id
    /// @param portfolio_id unique id of the portfolio
    /// @return sp to portfolio if exists
    shared_ptr<Portfolio> find_portfolio(const string &portfolio_id);

    /// @brief evaluate the portfolio on open or close
    /// @param on_close are we at close of the candle
    void evaluate(bool on_close);

    /// @brief recursivly populate PortfolioHistory object with current portfolio values
    void update();

    /// @brief set the portfolio event tracer when/if it is registered
    void set_event_tracer(shared_ptr<EventTracer> event_tracer_){this->event_tracer = event_tracer_;}

    /// @brief adjust nlv by amount, allows trades to adjust source portfolio values
    /// @param nlv_adjustment adjustment size
    void nlv_adjust(double nlv_adjustment) {this->nlv += nlv_adjustment;};
    void cash_adjust(double cash_adjustment) {this->cash += cash_adjustment;};
    void unrealized_adjust(double unrealized_adjustment) {this->unrealized_pl += unrealized_adjustment;};

    /// @brief generate and send nessecary orders to completely exist position by asset id (including all child portfolios)
    /// @param orders to vector to hold inverse orders
    std::optional<std::vector<order_sp_t>> generate_order_inverse( 
        const string & asset_id,
        bool send_orders,
        bool send_collapse);

    /**
     * @brief place a new order that when executed will create or modify a position to the given size
     * 
     * @param asset_id the unique id of the asset 
     * @param size the size of the target to place (make sure to see order_target_type)
     * @param strategy_id unique id of the strategy placing the order
     * @param epsilon the minimum pct difference in new size relatvie to exisitng to where the order is executed eg (.01)
     * @param order_target_type type of order target, raw units, dollars, or pct of nlv
     * @param order_execution_type  order execution type
     * @param trade_id unique id of the trade to adjust (-1 defaults to new trade)
     */
    void order_target_size(const string &asset_id, double size,
                           const string &strategy_id,
                           double epsilon,
                           OrderTargetType order_target_type,
                           OrderExecutionType order_execution_type = LAZY,
                           int trade_id = -1
    );

    /**
     * @brief generate new orders to get portfolio allocation to match the passed allocation dictionary
     * 
     * @param allocations           dictionary mapping asset id to portfolio allocation
     * @param strategy_id           unique id of the strategy placing the order
     * @param epsilon               the minimum pct difference in new size relatvie to exisitng to where the order is executed eg (.01)
     * @param order_execution_type  order exectuion type (lazy or eager)
     * @param order_target_type     type of allocation, raw units, dollars, or pct of nlv
     * @param clear_missing         clear positions that do not have an allocation
     */
    void order_target_allocations(
        py::dict allocations, 
        const string &strategy_id,
        double epsilon, 
        OrderExecutionType order_execution_type = OrderExecutionType::LAZY,
        OrderTargetType order_target_type = OrderTargetType::PCT,
        bool clear_missing = true
    );

    /**
     * @brief place a new market order
     * 
     * @param asset_id unique id of the underlying asset
     * @param units number of units to buy/sell
     * @param strategy_id unique id of the strategy
     * @param order_execution_type execution type of the order
     * @param trade_id unique id of the trade (-1 defaults to new trade)
     */
    void place_market_order(const string &asset_id, double units,
                            const string &strategy_id,
                            OrderExecutionType order_execution_type = LAZY,
                            int trade_id = -1);

    /**
     * @brief place a new limit order
     * 
     * @param asset_id unique id of the underlying asset
     * @param units number of units to buy/sell
     * @param limit the limit price of the new order
     * @param strategy_id unique id of the strategy
     * @param order_execution_type execution type of the order
     * @param trade_id unique id of the trade (-1 defaults to new trade)
     */
    void place_limit_order(const string &asset_id, double units, double limit,
                           const string &strategy_id,
                           OrderExecutionType order_execution_type = LAZY,
                           int tade_id = -1);

    /**
     * @brief close position by asset id, if no id is passed all positions are closed
     * 
     * @param asset_id asset id to close positions of, default is to close all
     */
    void py_close_position(const string& = "");

    /**
     * @brief add cash to the portfolio, adding cash propogates up portfolio tree
     *  cash added before the simulation begins will affect starting cash set point. 
     *
     * @param cash amount of cash to add
     */
    void add_cash(double cash);

    /**
     * @brief Get the portfolio id of the object
     * 
     * @return const string& unique id of the portfolio
     */
    const string & get_portfolio_id() const {return this->portfolio_id;}

    /**
     * @brief does the portfolio contain any positions (not including parent portfolios)
     * 
     * @return true
     * @return false 
     */
    bool is_empty() const {return this->positions_map.size() > 0;}

    /**
    * @brief add new tracer to the portfolio
    * 
    * @param tracer_type the tracer type of the new tracer, used for static casting to child classes
    */
    void add_tracer(PortfolioTracerType tracer_type);

    /**
     * @brief Get a tracer object from the portfolio
     * 
     * @param tracer_type type of the tracer
     * @return shared_ptr<PortfolioTracer> shared pointer to the portfolio tracer
     */
    shared_ptr<PortfolioTracer> get_tracer(PortfolioTracerType tracer_type);

    /**
     * @brief generate a consolidated order history for all child portfolios this will
     *        search through child portfolios and look for event tracers and copy the orders
     *        into the new vector 
     * @param orders reference to vector holding the consolidated order feed
     */
    void consolidate_order_history(vector<shared_ptr<Order>>& orders);

private:
    /// unique id of the portfolio
    string portfolio_id;

    /// logging level
    int logging;

    /// is the portfolio built
    bool is_built = false;

    /// smart pointer to parent_portfolio
    Portfolio* parent_portfolio = nullptr;

    /// mapping between sub portfolio id and potfolio smart poitner
    portfolios_map_t portfolio_map;

    /// mapping between asset id and position smart pointer
    positions_map_t positions_map;

    /// smart pointer to exchanges map
    exchanges_sp_t exchange_map;

    /// smart pointer to broker map
    brokers_sp_t brokers;

    /// smart pointer to portfolio history object
    shared_ptr<PortfolioHistory> portfolio_history;

    /// smart pointer to event tracer (nullptr if not registered)
    shared_ptr<EventTracer> event_tracer;

    /// fixed point cash held by the portfolio
    double cash;

    /// starting cash of the portfolio
    long long starting_cash;

    /// fixed point net liquidation value of the portfolio
    long long nlv;

    /// unrealized_pl of the portfolio
    double unrealized_pl = 0;

    /// @brief modify an existing postion based on a filled order
    /// @param filled_order ref to a sp to a filled order
    void modify_position(order_sp_t filled_order);

    /// @brief open a new position based on either filled order or new trade
    /// @param open_obj sp to either a new trade or a new order
    template<typename T>
    void open_position(T open_obj, bool adjust_cash);

    /// @brief close an existing position based on a filled order
    /// @param filled_order ref to a sp to order that has been filled
    void close_position(order_sp_t filled_order);

    /// @brief cancel all order for child trades in a position
    /// @param position_sp ref to sp of a position
    void position_cancel_order(position_sp_t position_sp);

    /// @brief cancel all open orders for a trade
    /// @param trade_sp ref to sp of a trade
    void trade_cancel_order(trade_sp_t &trade_sp);

    /// @brief propogate a new trade up the portfolio tree
    void propogate_trade_open_up(trade_sp_t trade_sp, bool adjust_cash);

    /// @brief propogate a trade close up the portfolio tree
    void propogate_trade_close_up(trade_sp_t trade_sp, bool adjust_cash);

    //============== logging helper functions ==============//
    void log_order_create(order_sp_t &filled_order);
    void log_order_fill(order_sp_t &filled_order);
    void log_position_open(shared_ptr<Position> &new_position);
    void log_position_close(shared_ptr<Position> &closed_position);
    void log_trade_close(shared_ptr<Trade> &closed_trade);
    void log_trade_open(trade_sp_t &new_trade);

};

template<typename T>
void Portfolio::open_position(T open_obj, bool adjust_cash)
{   
    #ifdef DEBUGGING
    fmt::print("Portfolio::open_position: {}\n",this->portfolio_id);
    #endif
    // build the new position and increment position counter used to set ids
    auto position = make_shared<Position>(open_obj);

    // insert the new position into the portfolio object
    this->positions_map.insert({open_obj->get_asset_id(), position});

    //propgate the new trade up portfolio tree
    auto trade_sp = position->get_trades().begin()->second;

    this->propogate_trade_open_up(trade_sp, adjust_cash);

    // adjust cash held by broker accordingly
    if(adjust_cash)
    {
        this->cash -= open_obj->get_units() * open_obj->get_average_price();
    }

    // log the position if needed
    if (this->logging > 0)
    {
        this->log_position_open(position);
        this->log_trade_open(trade_sp);
    }

    #ifdef DEBUGGING
    fmt::print("Portfolio::open_position: {} done\n",this->portfolio_id);
    #endif
}

class PortfolioTracer
{   
public:
    /// Tracer constructor
    PortfolioTracer(Portfolio* parent_portfolio_){this->parent_portfolio = parent_portfolio_;};

    /// Tracer default desctructor
    virtual ~PortfolioTracer() = default;

    /// pure virtual step function to be called on new step
    virtual void step() = 0;

    /// pure virtual function to get tracer type
    virtual PortfolioTracerType tracer_type() const = 0;

    // pure virtual function to build the tracer
    virtual void build(size_t portfolio_eval_length) = 0;

    // pure virtual function to reset the tracer
    virtual void reset() = 0;

protected:
    /// pointer to the parent portfolio
    Portfolio* parent_portfolio;
};

class ValueTracer : public PortfolioTracer
{
public:
    /// ValueTracer constructor
    ValueTracer(Portfolio* parent_portfolio_) : PortfolioTracer(parent_portfolio_){}

    /// nlv container
    std::vector<double> nlv_history;

    /// historical cash values of the portfolio
    vector<double> cash_history;

    /// Tracer type
    PortfolioTracerType tracer_type() const {return PortfolioTracerType::Value;}
    
    /// step function
    void step()
    {
        this->cash_history.push_back(this->parent_portfolio->get_cash());
        this->nlv_history.push_back(this->parent_portfolio->get_nlv());
    };

    /// build function, reserve space for vector to prevent extra
    void build(size_t portfolio_eval_length){
        this->nlv_history.reserve(portfolio_eval_length);
        this->cash_history.reserve(portfolio_eval_length);
    }

    /// build function
    void reset(){
        this->nlv_history.clear();
        this->cash_history.clear();
    }

    /// @brief get the historical net liquidation values of the portfolio
    py::array_t<double> get_nlv_history(){
        return to_py_array(
        this->nlv_history.data(),
        this->nlv_history.size(),
        true);
    }
        
    /// @brief get the historical cash values of the portfolio
    py::array_t<double> get_cash_history(){
        return to_py_array(
        this->cash_history.data(),
        this->cash_history.size(),
        true);
    }
};

class EventTracer : public PortfolioTracer
{
public:
    /// ValueTracer constructor
    EventTracer(Portfolio* parent_portfolio_) : PortfolioTracer(parent_portfolio_){};

    void remember_order( shared_ptr<Order> event){this->orders.push_back(event);}
    void remember_trade( shared_ptr<Trade> event){this->trades.push_back(event);}
    void remember_position( shared_ptr<Position> event){this->positions.push_back(event);}

    /// tracer type
    PortfolioTracerType tracer_type() const {return PortfolioTracerType::Event;}

    /// empty stepper
    void step(){}

    /// build event tracer
    void build(size_t portfolio_eval_length){}

    /// reset event tracer
    void reset(){
        this->orders.clear();
        this->trades.clear();
        this->positions.clear();
    }

    vector<shared_ptr<Order>>& get_order_history(){return this->orders;};
    vector<shared_ptr<Trade>>& get_trade_history(){return this->trades;};
    vector<shared_ptr<Position>>& get_position_history(){return this->positions;};

private:
    /// history of all orders
    vector<shared_ptr<Order>> orders;

    /// history of all trades
    vector<shared_ptr<Trade>> trades;

    /// history of all positions
    vector<shared_ptr<Position>> positions;
};

class PortfolioHistory{
public:
    /// portfolio history constructor
    PortfolioHistory(Portfolio* parent_portfolio_);
    
    /** @private
     * @brief add a new tracer to a portfolio history object, called through portfolio object
     * 
     * @param tracer_type type of the new tracer
     */
    void add_tracer(PortfolioTracerType tracer_type);
    
    /// find portfolio tracer by id
    shared_ptr<PortfolioTracer> get_tracer(PortfolioTracerType tracer_type);

    /// build portfolio history tracers    
    void build(size_t portfolio_eval_length);

    /// reset the portfolio history tracers
    void reset(bool clear_history);

    /// pointer to the parent portfolio
    Portfolio* parent_portfolio;

    /// update historical values with current snapshot
    void update();

    ///is the portfolio history built
    bool is_built;

    /// container of portfolio tracers applied to this portfolio
    vector<shared_ptr<PortfolioTracer>> tracers;

};


#endif // ARGUS_PORTFOLIO_H