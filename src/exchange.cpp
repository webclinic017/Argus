//
// Created by Nathan Tormaschy on 4/18/23.
//

#include "../include/exchange.h"

using namespace std;

void Exchange::register_asset(shared_ptr<Asset> asset) {
    string asset_id = asset->get_asset_id();

    if(this->market.contains(asset_id)){
        throw runtime_error("asset already exists");
    }
    else{
        this->market.emplace(asset_id, asset);
        this->market_view.emplace(asset_id, nullptr);
    }

}