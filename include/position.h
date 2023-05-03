//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_POSITION_H
#define ARGUS_POSITION_H
#include <optional>
#include <string>
#include <tsl/robin_map.h>

class Trade;
class Order;
class Position;
class Portfolio;

#include "trade.h"

using namespace std;

class Position
{
private:
    /// unique id of the position
    unsigned int position_id;

    /// unique id of the underlying asset of the position
    string asset_id;

    /// unique id of the exchange the underlying asset is on
    string exchange_id;

    /// net liquidation value of the position
    double nlv; 

    /// closing price of the position
    double close_price = 0;

    /// last price the position was evaluated at
    double last_price = 0;

    /// unrealized pl of the position
    double unrealized_pl = 0;

    /// realized pl of the position
    double realized_pl = 0;

    /// time the position was opened
    long long position_open_time;

    /// time the position was closed
    long long position_close_time = 0;

    /// number of bars the positions has been held for
    unsigned int bars_held = 0;

    tsl::robin_map<unsigned int, shared_ptr<Trade>> trades;

public:
    /// smart pointer position typedef
    using position_sp_t = std::shared_ptr<Position>;
    using order_sp_t = std::shared_ptr<Order>;
    using trade_sp_t = std::shared_ptr<Trade>;

    /// position constructors
    Position(order_sp_t filled_order);
    Position(trade_sp_t trade);

    auto get_mem_address(){return reinterpret_cast<std::uintptr_t>(this); }

    /// close the position out at given time and price
    /// \param market_price price the trade was closed out at
    /// \param position_close_time time the trade was closed out at
    void close(double market_price, long long position_close_time);


    trade_sp_t adjust_order(order_sp_t filled_order,  Portfolio* portfolio);
    
    trade_sp_t adjust_trade(trade_sp_t new_trade);

    /// how many units in the position
    double units;

    /// average price of the position
    double average_price = 0;

     /// is the position currently open
    bool is_open;

    /// @brief set the id of a position
    void set_position_id(unsigned int position_id_){this->position_id = position_id_;}

    /// @private
    /// @brief is the position currently open
    /// @brief set the wether the position is open
    void set_is_open(bool is_open_) {this->is_open = is_open_;}

    /// @brief set the position open status
    /// @return is the position currently open
    bool get_is_open() const {return this->is_open;}

    /// @brief get the positions net liquidation value as last calculated
    /// @return net liquidation value of the position
    double get_nlv() const {return this->nlv;}

    /// @brief get the positions unrealized pl
    double get_unrealized_pl() const {return this->unrealized_pl;}

    ///@brief get the number of trades in the position
    ///@return return the number of trades in the position
    unsigned int get_trade_count() const {return this->trades.size();}

    /// get the id of the position
    /// @return position id
    [[nodiscard]] unsigned int get_position_id() const { return this->position_id; };

    /// get the average price of the position
    /// @return position's average price
    [[nodiscard]] double get_average_price() const { return this->average_price; };

    /// get the closing price of the position 
    /// @return the closing price of the position
    [[nodiscard]] double get_close_price() const { return this->close_price; };

    /// get the id of the exchange the position's underlying asset is on
    /// \return id of the exchange the position's underlying asset is on
    string const & get_exchange_id() { return this->exchange_id; }

    /// get the id of the position's underlying asset
    /// \return position's asset id
    [[nodiscard]] string get_asset_id() const { return this->asset_id; };

    /// get total number of units in the position
    /// \return number of units in the position
    [[nodiscard]] double get_units() const { return this->units; }

    /// get the last price the position was valued at
    [[nodiscard]] double get_last_price() const {return this->last_price;}

    /// get the time the position was opened
    /// \return position open time
    [[nodiscard]] long long get_position_open_time() const { return this->position_open_time; };

    /// get the time the position was opened
    /// \return position open time
    [[nodiscard]] long long get_position_close_time() const { return this->position_close_time; };

    /// get a smart pointer to child trade
    std::optional<trade_sp_t> get_trade(unsigned int trade_id);

    /// @private
    /// get a reference to the hash map containing the underlying trades of the position
    /// \return reference to the hash map containing the underlying trades of the position
    tsl::robin_map<unsigned int, trade_sp_t> &get_trades() { return this->trades; }

    /// @brief generate the inverse orders needed to close out a position, (MARKET_ORDERS)
    /// @param ref to vector to hold inverse orders
    void generate_order_inverse(std::vector<order_sp_t>& orders);

    /// @private
    /// evaluate a position and it's child trades at the given market price
    inline void evaluate(double market_price, bool on_close)
    {
        this->last_price = market_price;
        this->unrealized_pl = this->units * (market_price - this->average_price);
        this->nlv = market_price * this->units;
        if (on_close)
        {
            this->bars_held++;
        }
    };
};
#endif // ARGUS_POSITION_H
