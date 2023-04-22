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

void Exchange::register_asset(const shared_ptr<Asset>& asset_) {
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

void Exchange::place_order(const shared_ptr<Order>& order) {
    this->open_orders.push_back(order);
}

void Exchange::process_market_order(shared_ptr<Order> &open_order, bool on_close){
    auto market_price = this->get_market_price(open_order->get_asset_id(), on_close);
    if(market_price == 0){
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    open_order->fill(market_price, this->exchange_time);

}

void Exchange::process_limit_order(shared_ptr<Order> &open_order, bool on_close){
    auto market_price = this->get_market_price(open_order->get_asset_id(), on_close);
    if(market_price == 0){
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() > 0) & (market_price <= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() < 0) & (market_price >= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_stop_loss_order(shared_ptr<Order> &open_order, bool on_close){
    auto market_price = this->get_market_price(open_order->get_asset_id(), on_close);
    if(market_price == 0){
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() < 0) & (market_price <= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() > 0) & (market_price >= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_take_profit_order(shared_ptr<Order> &open_order, bool on_close){
    auto market_price = this->get_market_price(open_order->get_asset_id(), on_close);
    if(market_price == 0){
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() < 0) & (market_price >= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() > 0) & (market_price <= open_order->get_limit())) {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_orders(bool on_close) {
    vector<size_t> filled_order_indecies;
    size_t index = 0;
    for(auto& order : this->open_orders){
        auto asset_id = order->get_asset_id();
        auto asset = this->market_view.at(asset_id);

        //check to see if asset is currently streaming
        if(!asset){
            continue;
        }

        //switch on order type and process accordingly
        switch (order->get_order_type()) {
            case MARKET_ORDER: {
                this->process_market_order(order, on_close);
                break;
            }
            case LIMIT_ORDER:{
                this->process_limit_order(order, on_close);
                break;
            }
            case STOP_LOSS_ORDER:{
                this->process_stop_loss_order(order, on_close);
                break;
            }
            case TAKE_PROFIT_ORDER:{
                this->process_take_profit_order(order, on_close);
                break;
            }
        }

        //if the order was filled mark its index
        if(order->get_order_state() == FILLED){
            filled_order_indecies.push_back(index);
        }
        index++;
    }

    //remove filled orders
    for(auto order_index : filled_order_indecies){
        std::swap(this->open_orders[order_index], this->open_orders.back());
        this->open_orders.pop_back();
    }
}

bool Exchange::get_market_view() {
    //if the current index is the last then return false, all assets listed on this exchange
    //are done streaming their data
    if(this->current_index == this->datetime_index_length){
        return false;
    }
    //loop through all the assets in this exchange. If the asset's next datetime is equal to the
    //next datetime of the exchange, then that asset will be seen in the market view.
    this->exchange_time = this->datetime_index[this->current_index];
    for (auto& _asset_pair : this->market) {
        auto asset_raw_pointer = _asset_pair.second.get();

        //get the asset's current time
        auto asset_datetime = asset_raw_pointer->get_asset_time();

        //asset has not reached the end of its data
        auto asset_id = asset_raw_pointer->get_asset_id();
        if(asset_datetime){
            if(*asset_datetime == this->exchange_time){
                //add asset to market view, step the asset forward in time
                this->market_view[asset_id] = asset_raw_pointer;
                asset_raw_pointer->step();
            }
            else{
                this->market_view[asset_id] = nullptr;
            }
        }
        //asset has reached the end of its data
        else{
            expired_asset_ids.push_back(asset_id);
        }
    }

    //move to next datetime and return true showing the market contains at least one
    //asset that is not done streaming
    this->current_index++;
    return true;
}

shared_ptr<Exchange> new_exchange(const string& exchange_id) {
    return std::make_shared<Exchange>(exchange_id);
}
