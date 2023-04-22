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

class Exchange{
private:
    ///is the exchange built
    bool is_built;

    ///unique id of the exchange
    string exchange_id;

    ///map between asset id and asset pointer
    tsl::robin_map<string,shared_ptr<Asset>> market;

    ///mapping for asset's available at the current moment;
    tsl::robin_map<string, Asset*> market_view;

    ///container for storing asset_id's that have finished streaming
    vector<string> expired_asset_ids;

    ///open orders on the exchange
    vector<shared_ptr<Order>> open_orders;

    ///current exchange time
    long long exchange_time;

    ///exchange datetime index
    long long* datetime_index;

    ///length of datetime index
    size_t datetime_index_length;

    ///current position in datetime index
    size_t current_index;

public:
    ///exchange constructor
    Exchange(string exchange_id_): exchange_id(std::move(exchange_id_)), is_built(false){};

    ///destructor for the exchange
    ~Exchange();

    /// Disable move constructor
    Exchange(Exchange&&) = delete;

    /// Disable move assignment operator
    Exchange& operator=(Exchange&&) = delete;

    ///build the exchange
    void build();

    ///build the market view, return false if all assets listed are done streaming
    bool get_market_view();

    ///register an asset on the exchange
    void register_asset(const shared_ptr<Asset>& asset);

    ///place order to the exchange
    void place_order(const shared_ptr<Order>& order);

    ///process open orders on the exchange
    void process_orders(bool on_close);

    ///process a market order currently open
    void process_market_order(shared_ptr<Order> &open_order, bool on_close);

    ///process a limit order currently open
    void process_limit_order(shared_ptr<Order> &open_order, bool on_close);

    ///process a stop loss order currently open
    void process_stop_loss_order(shared_ptr<Order> &open_order, bool on_close);

    ///process a take profit order currently open
    void process_take_profit_order(shared_ptr<Order> &open_order, bool on_close);


    ///build a new asset on the exchange
    std::shared_ptr<Asset> new_asset(const string& asset_id);

    ///get smart pointer to existing asset on the exchange
    std::shared_ptr<Asset> get_asset(const string& asset_id);

    ///get numpy array read only view into the exchange's datetime index
    py::array_t<long long> get_datetime_index_view();

    ///get read only pointer to datetime index
    long long * get_datetime_index(){return this->datetime_index;}

    ///get read exchange current time
    long long get_datetime(){return this->datetime_index[this->current_index];}

    ///is the exchange built yet
    [[nodiscard]] bool get_is_built() const {return this->is_built;}

    /// return the number of rows in the asset
    [[nodiscard]] size_t get_rows() const {return this->datetime_index_length;}

    inline double get_market_price(const string& asset_id, bool on_close){
        //get pointer to asset, nullptr if asset is not currently streaming
        auto asset_raw_pointer = this->market_view.at(asset_id);
        if(asset_raw_pointer){
            return asset_raw_pointer->get_market_price(on_close);
        }
        else{
            return 0.0;
        }
    }

};

///function for creating a shared pointer to a asset
shared_ptr<Exchange> new_exchange(const string &exchange_id);

#endif //ARGUS_EXCHANGE_H
