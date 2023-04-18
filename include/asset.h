//
// Created by Nathan Tormaschy on 4/17/23.
//

#ifndef ARGUS_ASSET_H
#define ARGUS_ASSET_H
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>

using namespace std;

class Asset{
private:
    ///has the asset been built
    bool is_built;

    ///unique id of the asset
    string asset_id;

    ///map between column name and column index
    unordered_map<string, size_t> headers;

    ///datetime index of the asset (ns epoch time stamp)
    vector<long long> datetime_index;

    ///underlying data of the asset
    vector<vector<double>> data;
public:
    ///asset constructor
    explicit Asset(string asset_id) :
        asset_id(std::move(asset_id)),
        is_built(false)
        {}

    ///return the id of an asset
    [[nodiscard]] string get_asset_id() const;

    ///test if the function is built
    [[nodiscard]] bool get_is_built() const;

    ///load in the headers of an asset from python list
    void load_headers(const vector<string>& headers);

};

shared_ptr<Asset> new_asset(const string& asset_id);

#endif //ARGUS_ASSET_H
