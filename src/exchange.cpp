//
// Created by Nathan Tormaschy on 4/18/23.
//

#include <iostream>
#include "exchange.h"
#include "asset.h"
#include "utils_array.h"
#include "settings.h"

using namespace std;

void Exchange::build() {
    #ifdef DEBUGGING
        printf("EXCHANGE: BUILDING EXCHANGE: %s\n", this->exchange_id.c_str());
    #endif

    //check to see if any assets exist
    if (this->market.empty()){
        throw std::runtime_error("no assets in the exchange to build");
    }
    //check to see if the exchange has been built before
    if(this->is_built){
        delete [] this->datetime_index;
    }

    auto datetime_index_ = container_sorted_union(
            this->market,
            [](const shared_ptr<Asset> &obj) { return obj->get_datetime_index(); },
            [](const shared_ptr<Asset> &obj) { return obj->get_rows(); }
    );

    this->datetime_index = get<0>(datetime_index_);
    this->datetime_index_length = get<1>(datetime_index_);
    this->is_built = true;

#ifdef DEBUGGING
    printf("EXCHANGE: EXCHANGE: %s BUILT\n", this->exchange_id.c_str());
#endif
}

Exchange::~Exchange(){
#ifdef DEBUGGING
    printf("MEMORY:   calling exchange %s DESTRUCTOR ON: %p \n",this->exchange_id.c_str(), this);
    printf("EXCHANGE: is built: %d", this->is_built);
#endif
    if(this->is_built){
        delete[] this->datetime_index;
    }
#ifdef DEBUGGING
    printf("MEMORY:   exchange %s DESTRUCTOR complete\n",this->exchange_id.c_str());
#endif
}

shared_ptr<Asset> Exchange::get_asset(const std::string& asset_id_) {
    try {
        return this->market.at(asset_id_);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Asset> Exchange::new_asset(const string& asset_id_){
    if(this->market.contains(asset_id_)){
        throw runtime_error("asset already exists");
    }
    auto asset = make_shared<Asset>(asset_id_);
    this->market.emplace(asset_id_,  make_shared<Asset>(*asset));
    this->market_view.emplace(asset_id_, nullptr);
    return asset;
}

void Exchange::register_asset(shared_ptr<Asset> asset_) {
    string asset_id = asset_->get_asset_id();

#ifdef DEBUGGING
    printf("EXCHANGE: exchange %s registering asset: %s \n", this->exchange_id, asset_id);
#endif

    if(this->market.contains(asset_id)){
        throw runtime_error("asset already exists");
    }
    else{
        this->market.emplace(asset_id, asset_);
        this->market_view.emplace(asset_id, nullptr);
    }
}

py::array_t<long long> Exchange::get_datetime_index_view() {
    if(!this->is_built){
        throw std::runtime_error("exchange is not built");
    }
    return to_py_array(
            this->datetime_index,
            this->datetime_index_length,
            true);
}

void Exchange::place_order(shared_ptr<Order> order) {
    this->open_orders.push_back(order);
}

shared_ptr<Exchange> new_exchange(const string& exchange_id) {
    return std::make_shared<Exchange>(exchange_id);
}
