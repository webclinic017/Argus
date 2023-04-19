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
#include <pybind11/stl.h>

namespace py = pybind11;

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
    long long* datetime_index{};

    ///underlying data of the asset
    double** data{};

    ///number of rows in the asset data
    size_t  rows;

    ///number of columns in the asset data
    size_t cols;

public:
    ///asset constructor
    explicit Asset(string asset_id);

    ///asset destructor
    ~ Asset();

    ///return the id of an asset
    [[nodiscard]] string get_asset_id() const;

    ///test if the function is built
    [[nodiscard]] bool get_is_built() const;

    ///load in the headers of an asset from python list
    void load_headers(const vector<string>& headers);

    ///load the asset data in from a pointer
    void load_data(const double *data, const long long * datetime_index, size_t rows, size_t cols);

    //load the asset data using a python buffer
    void py_load_data(const py::buffer& data, const py::buffer& datetime_index, size_t rows, size_t cols);

    ///get data point from asset
    [[nodiscard]] double get(const string& column, size_t row_index) const;
};

shared_ptr<Asset> new_asset(const string& asset_id);



#endif //ARGUS_ASSET_H
