//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <execution>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "fmt/core.h"
#include "pybind11/pytypes.h"

#include "exchange.h"
#include "asset.h"
#include "utils_array.h"
#include "settings.h"

using namespace std;

using asset_sp_t = Asset::asset_sp_t;

Exchange::Exchange(std::string exchange_id_, int logging_) : exchange_id(std::move(exchange_id_)),
                                                             is_built(false),
                                                             on_close(false),
                                                             logging(logging_)
{
    this->datetime_index = new long long[0];
    this->current_index = 0;
    this->datetime_index_length = 0;
    this->exchange_time = 0;
}

void Exchange::build()
{
#ifdef DEBUGGING
    printf("EXCHANGE: BUILDING EXCHANGE: %s\n", this->exchange_id.c_str());
#endif

    // check to see if any assets exist
    if (this->market.empty())
    {
        throw std::runtime_error("no assets in the exchange to build");
    }
    // check to see if the exchange has been built before
    if (this->is_built)
    {
        delete[] this->datetime_index;
    }

    this->candles = 0;

    auto datetime_index_ = container_sorted_union(
        this->market,
        [](const shared_ptr<Asset> &obj)
        { return obj->get_datetime_index(true); },
        [](const shared_ptr<Asset> &obj)
        { return obj->get_rows() - obj->warmup; });

    this->datetime_index = get<0>(datetime_index_);
    this->datetime_index_length = get<1>(datetime_index_);

    for(auto& asset_pair : this->market){
        auto asset = asset_pair.second;
        
        // test to see if asset is alligned with the exchage's datetime index
        // makes updating market view faster
        if(asset->get_rows() == this->datetime_index_length){
            asset->is_alligned = true;
            this->market_view[asset->get_asset_id()] = asset.get();
        }
        else{
            asset->is_alligned = false;
        }

        this->candles+= asset->get_rows();

    }

    this->is_built = true;

#ifdef DEBUGGING
    printf("EXCHANGE: EXCHANGE: %s BUILT\n", this->exchange_id.c_str());
#endif
}

void Exchange::reset_exchange()
{
    this->current_index = 0;
    this->market_view.clear();

    // reset assets still in the market
    for(auto & asset_pair : this->market)
    {   
        auto asset_sp = asset_pair.second;
        asset_sp->reset_asset();
        if(asset_sp->is_alligned)
        {
            this->market_view[asset_sp->get_asset_id()] = asset_sp.get();
        }
    }
    // reset assets that were expired and bring them back in to view
    for(auto & asset_sp : this->expired_assets)
    {   
        asset_sp->reset_asset();
        this->market.insert({asset_sp->get_asset_id(), asset_sp});
        if(asset_sp->is_alligned)
        {
            this->market_view[asset_sp->get_asset_id()] = asset_sp.get();
        }
    }
    // reset market allignment
    for(auto & asset_pair : this->market)
    {   
        auto asset = asset_pair.second;
        if(asset->is_alligned)
        {
            this->market_view[asset->get_asset_id()] = asset.get();
        }
    }
    this->expired_assets.clear();
    this->open_orders.clear();
}

Exchange::~Exchange()
{
#ifdef DEBUGGING
    printf("MEMORY:   calling exchange %s DESTRUCTOR ON: %p \n", this->exchange_id.c_str(), this);
    printf("EXCHANGE: is built: %d", this->is_built);
#endif
    delete[] this->datetime_index;
#ifdef DEBUGGING
    printf("MEMORY:   exchange %s DESTRUCTOR complete\n", this->exchange_id.c_str());
#endif
}

shared_ptr<Asset> Exchange::get_asset(const std::string &asset_id_)
{
    try
    {
        return this->market.at(asset_id_);
    }
    catch (const std::out_of_range &e)
    {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
}

shared_ptr<Asset> Exchange::new_asset(const string &asset_id_, const string &broker_id)
{
    if (this->market.count(asset_id_))
    {
        throw runtime_error("asset already exists");
    }
    auto asset = make_shared<Asset>(asset_id_, this->exchange_id, broker_id);
    this->market.emplace(asset_id_, make_shared<Asset>(*asset));
    this->market_view.emplace(asset_id_, nullptr);
    return asset;
}

void Exchange::register_asset(const shared_ptr<Asset> &asset_)
{
    string asset_id = asset_->get_asset_id();

#ifdef DEBUGGING
    fmt::print("EXCHANGE: exchange {} registering asset: {} \n", this->exchange_id, asset_id);
#endif

    if (this->market.count(asset_id))
    {
        throw runtime_error("asset already exists");
    }
    else
    {
        this->market.emplace(asset_id, asset_);
        this->market_view.emplace(asset_id, nullptr);
    }
}

py::array_t<long long> Exchange::get_datetime_index_view()
{
    if (!this->is_built)
    {
        throw std::runtime_error("exchange is not built");
    }
    return to_py_array(
        this->datetime_index,
        this->datetime_index_length,
        true);
}

void Exchange::place_order(shared_ptr<Order> &order_)
{   
    // set the time that the order was placed on the exchange
    order_->set_order_creat_time(this->exchange_time);

    // process order, either fill it or add it to the open order
    this->process_order(order_);
}

void Exchange::process_market_order(shared_ptr<Order> &open_order)
{
    auto market_price = this->get_market_price(open_order->get_asset_id());
    if (market_price == 0)
    {
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    open_order->fill(market_price, this->exchange_time);
}

void Exchange::process_limit_order(shared_ptr<Order> &open_order)
{
    auto market_price = this->get_market_price(open_order->get_asset_id());
    if (market_price == 0)
    {
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() > 0) & (market_price <= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() < 0) & (market_price >= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_stop_loss_order(shared_ptr<Order> &open_order)
{
    auto market_price = this->get_market_price(open_order->get_asset_id());
    if (market_price == 0)
    {
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() < 0) & (market_price <= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() > 0) & (market_price >= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_take_profit_order(shared_ptr<Order> &open_order)
{
    auto market_price = this->get_market_price(open_order->get_asset_id());
    if (market_price == 0)
    {
        throw std::invalid_argument("received order for which asset is not currently streaming");
    }
    if ((open_order->get_units() < 0) & (market_price >= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
    else if ((open_order->get_units() > 0) & (market_price <= open_order->get_limit()))
    {
        open_order->fill(market_price, this->exchange_time);
    }
}

void Exchange::process_order(shared_ptr<Order> &order)
{
    auto asset_id = order->get_asset_id();
    auto asset = this->market_view.at(asset_id);

    // check to see if asset is currently streaming
    if (!asset)
    {   
        ARGUS_RUNTIME_ERROR(fmt::format("failed to find asset: {} in market view",asset_id));
    }

    // switch on order type and process accordingly
    switch (order->get_order_type())
    {
    case MARKET_ORDER:
    {
        this->process_market_order(order);
        break;
    }
    case LIMIT_ORDER:
    {
        this->process_limit_order(order);
        break;
    }
    case STOP_LOSS_ORDER:
    {
        this->process_stop_loss_order(order);
        break;
    }
    case TAKE_PROFIT_ORDER:
    {
        this->process_take_profit_order(order);
        break;
    }
    }

    // if the order is still pending then set to open ad push to open order vector
    if (order->get_order_state() == PENDING)
    {
        order->set_order_state(OPEN);
        this->open_orders.push_back(order);
    }
}

void Exchange::process_orders()
{
    // iterate over open order and process any filled orders
    for (auto it = this->open_orders.begin(); it != this->open_orders.end();)
    {
        auto order = *it;

        // process the order
        this->process_order(order);

        // if order was filled remove from open orders, else move to next
        if (order->get_order_state() == FILLED)
        {
            it = this->open_orders.erase(it);
        }
        else
        {
            it++;
        };
    }
}

optional<vector<asset_sp_t>*> Exchange::get_expired_assets(){
    if(this->expired_assets.size() == 0){
        return std::nullopt;
    }
    else{
        return & this->expired_assets;
    }
};

void Exchange::move_expired_assets(){
    if(this->expired_assets.size() == 0){
        return;
    }
    else{
        for(const auto & asset : this->expired_assets){
            auto asset_id = asset->get_asset_id();
            
            //remove asset from market and market view
            this->market_view.erase(asset_id);
            this->market.erase(asset_id);
        }
    }
}

void Exchange::goto_datetime(long long datetime)
{
    //goto date is beyond the datetime index
   if(datetime >= this->datetime_index[this->datetime_index_length-1])
    {
        this->current_index = this->datetime_index_length;
    }
    
    // search for datetime in the index
    for(int i = this->current_index; i < this->datetime_index_length; i++)
    {   
        //is >= right?
        if(this->datetime_index[i] >= datetime)
        {
            this->current_index = i;
            return;
        }
    }

    throw std::runtime_error("failed to find datetime in exchange goto");    
}

bool Exchange::get_market_view()
{
    // if the current index is the last then return false, all assets listed on this exchange
    // are done streaming their data
    if (this->current_index == this->datetime_index_length)
    {
        return false;
    }
    
    // set exchange time to compare to assets
    this->exchange_time = this->datetime_index[this->current_index];

    // Define a lambda function that processes each asset
    auto process_asset = [&](auto& _asset_pair) {
        //access raw pointer
        auto asset_raw_pointer = _asset_pair.second.get();

        // if asset is alligned to exchange just step forward in time, clean up if needed 
        if(asset_raw_pointer->is_alligned){
            asset_raw_pointer->step();
            if(asset_raw_pointer->is_last_view()){
                expired_assets.push_back(_asset_pair.second);
            }
            return;
        }

        // get the asset's current time and id
        auto asset_datetime = asset_raw_pointer->get_asset_time();
        auto asset_id = asset_raw_pointer->get_asset_id(); 
        if (asset_datetime && *asset_datetime == this->exchange_time)
        {   
            // add asset to market view, step the asset forward in time
            this->market_view[asset_id] = asset_raw_pointer;
            asset_raw_pointer->step();

            // test to see if this is the last row of data for the asset
            if(asset_raw_pointer->is_last_view()){
                expired_assets.push_back(_asset_pair.second);
            } 
        }
        else
        {
            this->market_view[asset_id] = nullptr;
        }
    };
    std::for_each(
        this->market.begin(), 
        this->market.end(), 
        process_asset);

    // move to next datetime and return true showing the market contains at least one
    // asset that is not done streaming
    this->current_index++;
    return true;
}

optional<double> Exchange::get_asset_feature(const string& asset_id, const string& column_name, int index){
    auto asset_sp = this->market_view.at(asset_id);
    
    #ifdef ARGUS_RUNTIME_ASSERT
    assert(asset_sp);
    #endif

    auto asset_value = asset_sp->get_asset_feature(column_name, index);
    return asset_value;
}

py::dict Exchange::get_exchange_feature(
    const string& column, 
    int row,
    ExchangeQueryType query_type,
    int N)
{
    size_t number_assets;
    if(N == -1)
    {
        number_assets = this->market_view.size();
    }
    else 
    {
        number_assets = static_cast<size_t>(N);
    }
    
    py::dict py_dict;
    // default query type implies just find all assets with the feature
    if(query_type == ExchangeQueryType::Default)
    {
        int i = 0;
        for(auto it = this->market_view.begin(); it != this->market_view.end(); it++)
        {
            if(i == number_assets)
            {
                return py_dict;
            }
            //check if asset is streaming
            auto asset_sp = it->second;
            if(asset_sp)
            {
                //place the value in the dict if the asset has that feature, else skip
                py_dict[it->first.c_str()] = asset_sp->get_asset_feature(column, row);
            }
            else
            {
                continue;        
            }
            i++;
        }
        return py_dict;
    }
    // query needs to be sorted, therefore we need to look at all possible assets
    std::vector<std::pair<std::string, double>> asset_pairs;
    for(auto it = this->market_view.begin(); it != this->market_view.end(); it++)
    {
        //check if asset is streaming
        auto asset_sp = it->second;
        if(asset_sp)
        {
            //place the value in the dict if the asset has that feature, else skip
            auto asset_value = asset_sp->get_asset_feature(column, row);
            asset_pairs.emplace_back(std::make_pair(it->first.c_str(),asset_value));
            
        }
    }
    // sort the asset feature pairs using the feature 
    std::sort(asset_pairs.begin(), asset_pairs.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    if(number_assets > asset_pairs.size())
    {
        number_assets = asset_pairs.size();
    }

    switch (query_type) {
        case ExchangeQueryType::NSmallest:
            for(size_t i = 0; i < number_assets; i++)
            {
                auto pair = asset_pairs[i];
                py_dict[pair.first.c_str()] = pair.second;
            }
            break;
        case ExchangeQueryType::NLargest:
            for(size_t i = 0; i < number_assets; i++)
            {
                auto pair = asset_pairs[asset_pairs.size()-i-1];
                py_dict[pair.first.c_str()] = pair.second;
            }
            break;
        case ExchangeQueryType::NExtreme: //skips integer reaminder (i.e. N=3 returns 2 assets)
            for(size_t i = 0; i < std::floor(number_assets/2); i++)
            {
                auto pair = asset_pairs[i];
                py_dict[pair.first.c_str()] = pair.second;
            }
            for(size_t i = 0; i < std::floor(number_assets/2); i++)
            {
                auto pair = asset_pairs[asset_pairs.size()-i-1];
                py_dict[pair.first.c_str()] = pair.second;
            }
            break;
        case ExchangeQueryType::Default:
            break;
    }
    return py_dict;
}

double ExchangeMap::get_market_price(const string& asset_id)
{
    auto& asset = this->asset_map.at(asset_id);
    auto exchange = this->exchanges.find(asset->exchange_id);
    return asset->get_market_price(exchange->second->on_close);
}

shared_ptr<Exchange> new_exchange(const string &exchange_id, int logging_)
{
    return std::make_shared<Exchange>(exchange_id, logging_);
}
