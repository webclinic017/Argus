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

    ///open orders on the exchange
    vector<shared_ptr<Order>> open_orders;

    ///current exchange time
    long long exchange_time;

    ///exchange datetime index
    long long* datetime_index;

    ///length of datetime index
    size_t datetime_index_length;

public:
    ///destructor for the exchange
    ~Exchange();

    ///build the exchange
    void build();

    ///register an asset on the exchange
    void register_asset(shared_ptr<Asset> asset);

    ///build a new asset on the exchange
    std::shared_ptr<Asset> new_asset(const string& asset_id);

    ///get smart pointer to existing asset on the exchange
    std::shared_ptr<Asset> get_asset(const string& asset_id);

    ///place order to the exchange
    void place_order(shared_ptr<Order> order);

    ///exchange constructor
    Exchange(string exchange_id_): exchange_id(std::move(exchange_id_)), is_built(false){};

    ///get read only view into the exchange's datetime index
    py::array_t<long long> get_datetime_index_view();

    ///is the exchange built yet
    [[nodiscard]] bool get_is_built() const {return this->is_built;}
};

///function for creating a shared pointer to a asset
shared_ptr<Exchange> new_exchange(const string &exchange_id);

#endif //ARGUS_EXCHANGE_H
