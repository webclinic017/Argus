//
// Created by Nathan Tormaschy on 4/18/23.
//

#include "exchange.h"
#include "asset.h"

using namespace std;

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
