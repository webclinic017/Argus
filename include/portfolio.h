#ifndef ARGUS_PORTFOLIO_H
#define ARGUS_PORTFOLIO_H

#include <memory>
#include <string>
#include <optional>
#include <sys/_types/_size_t.h>
#include <unordered_map>
#include <fmt/core.h>
#include <mutex>

class Broker;

#include "threaded.h"
#include "order.h"
#include "trade.h"
#include "position.h"
#include "account.h"
#include "history.h"
#include "exchange.h"

class PortfolioHistory;

class Portfolio : public std::enable_shared_from_this<Portfolio>
{
public:

    using position_sp_t = Position::position_sp_t;
    using exchanges_sp_t = ExchangeMap::exchanges_sp_t;
    using trade_sp_t = Trade::trade_sp_t;
    using order_sp_t = Order::order_sp_t;

    typedef ThreadSafeSharedPtr<Portfolio> portfolio_sp_threaded_t;
    typedef std::unordered_map<std::string, position_sp_t> positions_map_t;
    typedef std::unordered_map<std::string, portfolio_sp_threaded_t> portfolios_map_t;
    typedef shared_ptr<std::unordered_map<string, shared_ptr<Broker>>> brokers_sp_t;

    /// portfolio constructor
    /// @param logging logging level
    /// @param cash    starting cash of the portfolio
    /// @param id      unique id of the portfolio
    Portfolio(
        int logging, 
        double cash, 
        string id, 
        shared_ptr<History> history_,
        Portfolio* parent_portfolio_,
        brokers_sp_t brokers_,
        exchanges_sp_t exchanges_
        );

    void build(size_t portfolio_eval_length);

    /// get the memory addres of the portfolio object
    auto get_mem_address(){return reinterpret_cast<std::uintptr_t>(this); }
    
    /// @brief the amount of cash held by the portfolio (recursive sum of all child portfolios)
    double get_cash() const {return this->cash;}

    /// @brief get the net liquidation as last calculated
    double get_nlv() const {return this->nlv;}

    /// @brief get the net liquidation as last calculated
    double get_unrealized_pl() const {return this->unrealized_pl;}
    
    /// @brief function to handle a order fill event
    /// @param filled_order a sp to a new filled order recieved from a broker
    void on_order_fill(order_sp_t filled_order);

    /// generate a iterator begin and end for the position map
    /// \return pair of iteratores, begin and end, for postion map
    auto get_iterator()
    {
        return std::make_pair(this->positions_map.begin(), this->positions_map.end());
    }

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

    /// set the parent portfolio pointer
    Portfolio* get_parent_portfolio(){return this->parent_portfolio;}

    /// add new sub portfolio to the portfolio
    /// @param portfolio_id portfolio id of the new sub portfolio
    /// @param portfolio smart pointer to sub portfolio
    void add_sub_portfolio(const string &portfolio_id, portfolio_sp_threaded_t portfolio);
    
    /// create a new sub portfolio to the parent
    /// @param portfolio_id portfolio id of the new sub portfolio
    /// @param amount of cash held in the new portfolio
    shared_ptr<Portfolio> create_sub_portfolio(const string &portfolio_id, double cash);

    /// @brief get smartpointer to a sub portfolio
    /// @param portfolio_id id of the sub portfolio
    /// @return smart pointer to the sub portfolio
    optional<portfolio_sp_threaded_t>  get_sub_portfolio(const string &portfolio_id);

    /// @brief recursively search through sub portfolios to find by portfolio id
    /// @param portfolio_id unique id of the portfolio
    /// @return sp to portfolio if exists
    shared_ptr<Portfolio> find_portfolio(const string &portfolio_id);

    /// @brief evaluate the portfolio on open or close
    /// @param on_close are we at close of the candle
    void evaluate(bool on_close);

    /// @brief recursivly populate PortfolioHistory object with current portfolio values
    void update();

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
    
    /// place an to target a certain size held be the portfolio it was placed to
    void order_target_size(const string &asset_id, double size,
                           const string &exchange_id,
                           const string &broker_id,
                           const string &strategy_id,
                           OrderTargetType order_target_type
                    );

    /// order placement wrappers exposed to python
    void place_market_order(const string &asset_id, double units,
                            const string &strategy_id,
                            OrderExecutionType order_execution_type = LAZY,
                            int trade_id = -1);

    void place_limit_order(const string &asset_id, double units, double limit,
                           const string &strategy_id,
                           OrderExecutionType order_execution_type = LAZY,
                           int tade_id = -1);

    const string & get_portfolio_id() const {return this->portfolio_id;}
    bool is_empty() const {return this->positions_map.size() > 0;}

private:
    /// unique id of the portfolio
    std::string portfolio_id;

    /// logging level
    int logging;

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

    /// position counter for position ids
    unsigned int position_counter = 0;

    /// trade counter shared
    unsigned int trade_counter = 0;

    /// cash held by the portfolio
    double cash;

    /// net liquidation value of the portfolio
    double nlv;

    /// unrealized_pl of the portfolio
    double unrealized_pl = 0;

    // shared pointer to history objects
    shared_ptr<History> history;

    /// @brief add new position to the map by asset id
    /// @param asset_id asset id of the position to add
    /// @param position new position smart pointer
    inline void add_position(const string &asset_id, position_sp_t position);

    /// @brief remove a position from the map by asset id
    /// @param asset_id asset id of the position to delete
    inline void delete_position(const string &asset_id);

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

    // logging helper functions
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
    position->set_position_id(this->position_counter);
    this->position_counter++;

    // insert the new position into the portfolio object
    this->add_position(open_obj->get_asset_id(), position);

    //propgate the new trade up portfolio tree
    auto trade_sp = position->get_trades().begin()->second;
    this->propogate_trade_open_up(trade_sp, adjust_cash);

    // adjust cash held by broker accordingly
    if(adjust_cash)
    {
        this->cash -= open_obj->get_units() * open_obj->get_average_price();
    }

    // log the position if needed
    #ifdef ARGUS_STRIP
    if (this->logging > 0)
    {
        this->log_position_open(position);
        this->log_trade_open(trade_sp);
    }
    #endif

    #ifdef DEBUGGING
    fmt::print("Portfolio::open_position: {} done\n",this->portfolio_id);
    #endif
}


class PortfolioHistory{
public:
    /// portfolio history constructor
    PortfolioHistory(Portfolio* parent_portfolio_): parent_portfolio(parent_portfolio_){};
    
    // allocate memory
    void build(size_t portfolio_eval_length){
        this->cash_history.reserve(portfolio_eval_length); 
        this->nlv_history.reserve(portfolio_eval_length); 
    }
    
    /// pointer to the parent portfolio
    Portfolio* parent_portfolio;

    /// historical cash values of the portfolio
    vector<double> cash_history;
    
    /// historical net liquidation values of the portfolio
    vector<double> nlv_history;

    py::array_t<double> get_cash_history(){
        return to_py_array(
        this->cash_history.data(),
        this->cash_history.size(),
        true);
    }

    py::array_t<double> get_nlv_history(){
        return to_py_array(
        this->nlv_history.data(),
        this->nlv_history.size(),
        true);
    }

    /// @brief update historical values with current snapshot
    void update(){
        this->cash_history.push_back(this->parent_portfolio->get_cash());
        this->nlv_history.push_back(this->parent_portfolio->get_nlv());
    }

    bool is_built;

};


#endif // ARGUS_PORTFOLIO_H