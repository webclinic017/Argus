//
// Created by Nathan Tormaschy on 4/17/23.
//

#ifndef ARGUS_ASSET_H
#define ARGUS_ASSET_H
#include <cstddef>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <tsl/robin_map.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "utils_array.h"

namespace py = pybind11;

using namespace std;

class Asset
{
public:
    typedef shared_ptr<Asset> asset_sp_t;

    /// asset constructor
    Asset(string asset_id, string exchange_id, string broker_id, size_t warmup = 0);

    /// asset destructor
    ~Asset();

    /// reset asset to start of data
    void reset_asset();

    /// unique id of the asset
    string asset_id;

    /// unique id of the exchange the asset is on
    string exchange_id;

    /// unique id of the broker the asset is listed on 
    string broker_id;

    /// is the asset's datetime index alligend with it's exchange
    bool is_alligned;

    /// warmup period, i.e. number of rows to skip
    size_t warmup = 0;

    /// is the the last row in the asset
    bool is_last_view() {return this->current_index == this->rows;};

    /// return the memory address of the underlying asset opbject
    auto get_mem_address(){return reinterpret_cast<std::uintptr_t>(this); }
    
    /// return the number of rows in the asset
    [[nodiscard]] size_t get_rows() const { return this->rows; }

    /// return the number of columns in the asset
    [[nodiscard]] size_t get_cols() const { return this->cols; }

    /// return the id of an asset
    [[nodiscard]] string get_asset_id() const;

    /// return pointer to the first element of the datetime index;
    [[nodiscard]] long long *get_datetime_index() const;

    /// test if the function is built
    [[nodiscard]] bool get_is_built() const;

    /// load in the headers of an asset from python list
    void load_headers(const vector<string> &headers);

    /// load the asset data in from a pointer, copy to dynamically allocated double**
    void load_data(const double *data, const long long *datetime_index, size_t rows, size_t cols);
    void load_view(double *data, long long *datetime_index, size_t rows, size_t cols);

    /// load the asset data using a python buffer
    void py_load_data(
        const py::buffer &data, 
        const py::buffer &datetime_index, 
        size_t rows, 
        size_t cols,
        bool is_view);

    /// get data point from current asset row
    [[nodiscard]] double c_get(const string &column) const;

    /// get data point from asset
    [[nodiscard]] double get(const string &column, size_t row_index) const;

    /// get the current index of the asset
    [[nodiscard]] double get_market_price(bool on_close) const;

    /// get the current datetime of the asset
    [[nodiscard]] long long *get_asset_time() const;

    /// get read only numpy array of the asset's datetime index
    py::array_t<long long> get_datetime_index_view();

    [[nodiscard]] double get_asset_feature(const string& column_name, int index = 0);

    /// step the asset forward in time
    void step();

private:
    /// has the asset been built
    bool is_built = false;

    /// does the asset own the underlying data pointer
    bool is_view = false;

    /// map between column name and column index
    tsl::robin_map<string, size_t> headers;

    /// datetime index of the asset (ns epoch time stamp)
    long long *datetime_index;

    /// underlying data of the asset
    double * data;

    /// @brief pointer to the current row
    double * row;

    /// number of rows in the asset data
    size_t rows;

    /// number of columns in the asset data
    size_t cols;

    /// index of the current row the asset is at
    size_t current_index;

    /// index of the open column;
    size_t open_column;

    /// index of the close column
    size_t close_column;

};

/// function for creating a shared pointer to a asset
std::shared_ptr<Asset> new_asset(
    const string &asset_id, 
    const string& exchange_id, 
    const string& broker_id,
    size_t warmup = 0    
);

/// function for identifying index locations of open and close column
tuple<::size_t, size_t> parse_headers(const vector<std::string> &columns);

#endif // ARGUS_ASSET_H
