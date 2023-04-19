//
// Created by Nathan Tormaschy on 4/18/23.
//

#ifndef ARGUS_EXCHANGE_H
#define ARGUS_EXCHANGE_H
#include <string>
#include <memory>
#include "unordered_map"
#include "asset.h"

using namespace std;

class Exchange{
private:
    ///is the exchange built
    bool is_built;

    ///unique id of the exchange
    string exchange_id;

    ///map between asset id and asset pointer
    unordered_map<string,shared_ptr<Asset>> market;

    ///mapping for asset's available at the current moment;
    unordered_map<string, Asset*> market_view;

    ///current exchange time
    long long exchange_time;

    ///exchange datetime index
    vector<long long> datetime_index;

public:
    ///register an asset on the exchange
    void register_asset(shared_ptr<Asset> asset);

};

#endif //ARGUS_EXCHANGE_H
