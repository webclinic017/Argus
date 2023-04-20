//
// Created by Nathan Tormaschy on 4/18/23.
//

#include "exchange.h"
#include "asset.h"
#include "utils_array.h"

using namespace std;

void Exchange::build() {
    //check to see if any assets exist
    if (this->market.empty()){
        throw std::runtime_error("no assets in the exchange to build");
    }
    //check to see if the exchange has been built before
    if(this->is_built){
        delete [] this->datetime_index;
    }

    this->datetime_index = new long long[1]();
    this->datetime_index_length = 1;

    //generate the sorted datetime index of all assets on the exchange
    for(const auto & it : this->market) {
        auto asset = it.second;

        //get sorted union of the two datetime indecies
        auto sorted_index_tuple = sorted_union(
                this->datetime_index,       asset->get_datetime_index(),
                this->datetime_index_length,asset->get_rows());

        auto sorted_index = get<0>(sorted_index_tuple);
        auto sorted_index_size = get<1>(sorted_index_tuple);

        //swap pointers between the new sorted union and the existing one
        std::swap(this->datetime_index , sorted_index);
        this->datetime_index_length = sorted_index_size;
        delete [] sorted_index;
    }

    this->is_built = true;
}

Exchange::~Exchange(){
    if(this->is_built){
        delete[] this->datetime_index;
    }
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

void Exchange::register_asset(shared_ptr<Asset> &asset_) {
    string asset_id = asset_->get_asset_id();

    if(this->market.contains(asset_id)){
        throw runtime_error("asset already exists");
    }
    else{
        this->market.emplace(asset_id, asset_);
        this->market_view.emplace(asset_id, nullptr);
    }
}

shared_ptr<Exchange> new_exchange(const string& exchange_id) {
    return std::make_shared<Exchange>(exchange_id);
}
