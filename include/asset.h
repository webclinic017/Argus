//
// Created by Nathan Tormaschy on 4/17/23.
//

#ifndef ARGUS_ASSET_H
#define ARGUS_ASSET_H
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
private:
    /// has the asset been built
    bool is_built;

    /// is the asset a view on borrowed data. If set to true the columns of the asset data are
    /// not allocated on the heap, the are simply set to point to a location owned by a different object.
    /// if that object goes out of scope before the asset and we still point to it, bad stuff happens.
    /// use with cautions, prevents copying of data and is much faster but asset no longer owns data
    bool is_view;

    /// unique id of the asset
    string asset_id;

    /// map between column name and column index
    tsl::robin_map<string, size_t> headers;

    /// datetime index of the asset (ns epoch time stamp)
    long long *datetime_index;

    /// underlying data of the asset
    double **data;

    /// number of rows in the asset data
    size_t rows;

    /// number of columns in the asset data
    size_t cols;

    /// index of the current row the asset is at
    size_t current_index;

    /// index of the open column;
    size_t open_column;

    ///index of the close column
    size_t close_column;

public:
    /// asset constructor
    explicit Asset(string asset_id);

    /// asset destructor
    ~Asset();

    /// return the number of rows in the asset
    [[nodiscard]] size_t get_rows() const {return this->rows;}

    /// return the number of columns in the asset
    [[nodiscard]] size_t get_cols() const {return this->cols;}

    /// return the id of an asset
    [[nodiscard]] string get_asset_id() const;

    /// return pointer to the first element of the datetime index;
    [[nodiscard]] long long* get_datetime_index() const;

    /// test if the function is built
    [[nodiscard]] bool get_is_built() const;

    /// load in the headers of an asset from python list
    void load_headers(const vector<string> &headers);

    /// load the asset data in from a pointer, copy to dynamically allocated double**
    void load_data(const double *data, const long long *datetime_index, size_t rows, size_t cols);

    /// load the asset data in from a pointer, set pointers to data owned by someone else
    void load_data_view(double *data, long long *datetime_index, size_t rows, size_t cols);

    /// load the asset data using a python buffer
    void py_load_data(const py::buffer &data, const py::buffer &datetime_index, size_t rows, size_t cols, bool is_view);

    /// get data point from asset
    [[nodiscard]] double get(const string &column, size_t row_index) const;

    /// get the current index of the asset
    [[nodiscard]] inline double get_market_price(bool on_close) const{
        if (on_close)
            return this->data[current_index - 1][this->close_column];
        else
            return this->data[current_index - 1][this->open_column];
    };

    /// get the current index of the asset
    [[nodiscard]] long long* get_asset_time() const;

    ///get read only numpy array of the asset's datetime index
    py::array_t<long long> get_datetime_index_view();

    ///step the asset forward in time
    void step(){this->current_index++;}

};

///function for creating a shared pointer to a asset
shared_ptr<Asset> new_asset(const string &asset_id);

///function for identifying index locations of open and close column
tuple<::size_t , size_t > parse_headers(const vector<std::string> &columns);

#endif // ARGUS_ASSET_H
