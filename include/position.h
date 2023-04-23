//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_POSITION_H
#define ARGUS_POSITION_H
#include <string>
#include <tsl/robin_map.h>

#include "trade.h"

using namespace std;

class Position
{
private:
    /// is the position currently open
    bool is_open;

    /// unique id of the position
    unsigned int position_id;

    /// unique id of the underlying asset of the position
    string asset_id;

    /// unique id of the exchange the underlying asset is on
    string exchange_id;

    /// how many units in the position
    double units;

    /// average price of the position
    double average_price;

    /// closing price of the position
    double close_price;

    /// last price the position was evaluated at
    double last_price;

    /// unrealized pl of the position
    double unrealized_pl;

    /// realized pl of the position
    double realized_pl;

    /// time the position was opened
    long long position_open_time;

    /// time the position was closed
    long long position_close_time;

    /// number of bars the positions has been held for
    unsigned int bars_held;

    /// counter for setting trade id's
    unsigned int trade_counter;

    tsl::robin_map<unsigned int, shared_ptr<Trade>> trades;

public:
    /// smart pointer position typedef
    using position_sp_t = std::shared_ptr<Position>;

    /// position constructor
    Position(shared_ptr<Order> &filled_order, unsigned int position_id);

    /// close the position out at given time and price
    /// \param market_price price the trade was closed out at
    /// \param position_close_time time the trade was closed out at
    void close(double market_price, long long position_close_time);

    /// adjust a position's size and adjust the child trade the order was placed to
    /// \param market_price price the adjustment order was filled at
    /// \param units number of units to increase
    /// \param position_change_time time the order was filled at
    /// \param trade_id id of the child trade to adjust
    /// \return smart pointer to the trade that was adjusted
    shared_ptr<Trade> adjust(shared_ptr<Order> &filled_order);

    /// get the id of the position
    /// \return position id
    [[nodiscard]] unsigned int get_position_id() const { return this->position_id; };

    /// get the id of the position's underlying asset
    /// \return position's asset id
    [[nodiscard]] string get_asset_id() const { return this->asset_id; };

    /// get the average price of the position
    /// \return position's average price
    [[nodiscard]] double get_average_price() const { return this->average_price; };

    /// get the id of the exchange the position's underlying asset is on
    /// \return id of the exchange the position's underlying asset is on
    string get_exchange_id() { return this->exchange_id; }

    /// get total number of units in the position
    /// \return number of units in the position
    [[nodiscard]] double get_units() const { return this->units; }

    /// get the time the position was opened
    /// \return position open time
    [[nodiscard]] long long get_position_open_time() const { return this->position_open_time; };

    /// get a smart pointer to child trade
    shared_ptr<Trade> get_trade(unsigned int trade_id) { return this->trades.at(trade_id); }

    /// get a reference to the hash map containing the underlying trades of the position
    /// \return reference to the hash map containing the underlying trades of the position
    tsl::robin_map<unsigned int, shared_ptr<Trade>> &get_trades() { return this->trades; }

    void new_trade(const shared_ptr<Trade> &trade);

    inline void evaluate(double market_price, bool on_close)
    {
        this->last_price = market_price;
        this->unrealized_pl = this->units * (market_price - this->average_price);
        if (on_close)
        {
            this->bars_held++;
        }
        for (auto &trade : this->trades)
        {
            trade.second->evaluate(market_price, on_close);
        }
    };
};

#endif // ARGUS_POSITION_H
