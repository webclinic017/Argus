//
// Created by Nathan Tormaschy on 4/18/23.
//

#ifndef ARGUS_EXCHANGE_H
#define ARGUS_EXCHANGE_H
#include <string>
#include <memory>
#include <utility>
#include <tsl/robin_map.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "asset.h"
#include "order.h"

#include "utils_array.h"

using namespace std;
namespace py = pybind11;

// forward exchange class definition for typedef
class Exchange;

typedef tsl::robin_map<string, shared_ptr<Exchange>> Exchanges;
typedef shared_ptr<Exchanges> exchanges_sp_t;

class Exchange
{
public:
    using asset_sp_t = Asset::asset_sp_t;

    /// exchange constructor
    Exchange(string exchange_id_, int logging_);

    /// destructor for the exchange
    ~Exchange();

    /// Disable move constructor
    Exchange(Exchange &&) = delete;

    /// Disable move assignment operator
    Exchange &operator=(Exchange &&) = delete;

    /// build the exchange
    void build();

    /// build the market view, return false if all assets listed are done streaming
    bool get_market_view();

    /// register an asset on the exchange
    void register_asset(const asset_sp_t &asset);

    /// process open orders on the exchange
    void process_orders();

    /// place order to the exchange
    void place_order(shared_ptr<Order> &order);

    void set_on_close(bool on_close_) { this->on_close = on_close_; }

    /// build a new asset on the exchange
    asset_sp_t new_asset(const string &asset_id);

    /// get smart pointer to existing asset on the exchange
    asset_sp_t get_asset(const string &asset_id);

    /// get numpy array read only view into the exchange's datetime index
    py::array_t<long long> get_datetime_index_view();

    /// get read only pointer to datetime index
    long long const * get_datetime_index() { return this->datetime_index; }

    /// get read exchange current time
    long long get_datetime() { return this->datetime_index[this->current_index]; }

    /// is the exchange built yet
    [[nodiscard]] bool get_is_built() const { return this->is_built; }

    /// return the number of rows in the asset
    [[nodiscard]] size_t get_rows() const { return this->datetime_index_length; }

    inline double get_market_price(const string &asset_id)
    {
        // get pointer to asset, nullptr if asset is not currently streaming
        auto asset_raw_pointer = this->market_view.at(asset_id);
        if (asset_raw_pointer)
        {
            return asset_raw_pointer->get_market_price(this->on_close);
        }
        else
        {
            return 0.0;
        }
    }
private:
    /// logging level
    int logging;

    /// is the exchange built
    bool is_built;

    /// is the close of a candle
    bool on_close;

    /// unique id of the exchange
    string exchange_id;

    /// map between asset id and asset pointer
    tsl::robin_map<string, asset_sp_t> market;

    /// mapping for asset's available at the current moment;
    tsl::robin_map<string, Asset *> market_view;

    /// container for storing asset_id's that have finished streaming
    vector<string> expired_asset_ids;

    /// open orders on the exchange
    vector<shared_ptr<Order>> open_orders;

    /// current exchange time
    long long exchange_time;

    /// exchange datetime index
    long long *datetime_index;

    /// length of datetime index
    size_t datetime_index_length;

    /// current position in datetime index
    size_t current_index;

    /// process open orders on the exchange
    void process_order(shared_ptr<Order> &open_order);

    /// process a market order currently open
    void process_market_order(shared_ptr<Order> &open_order);

    /// process a limit order currently open
    void process_limit_order(shared_ptr<Order> &open_order);

    /// process a stop loss order currently open
    void process_stop_loss_order(shared_ptr<Order> &open_order);

    /// process a take profit order currently open
    void process_take_profit_order(shared_ptr<Order> &open_order);
};

/// function for creating a shared pointer to a asset
shared_ptr<Exchange> new_exchange(const string &exchange_id);

#endif // ARGUS_EXCHANGE_H
