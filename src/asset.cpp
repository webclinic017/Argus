#include <string>
#include <memory>
#include <pybind11/stl.h>
#include <stdexcept>

#include "../include/asset.h"

namespace py = pybind11;
using namespace std;

string Asset::get_asset_id() const{
    return this->asset_id;
}

bool Asset::get_is_built() const{
    return this->is_built;
}

void Asset::load_headers(const vector<std::string> &columns) {
    for (size_t i = 0; const auto& column_name : columns) {
        this->headers.emplace(column_name, i);
    }
}

void Asset::load_data(const double *data_, const long long *datetime_index_, size_t rows_, size_t cols_) {
    if (this->is_built){
        throw runtime_error("asset is already built");
    }
    for (int i = 0; i < cols_; i++) {
        auto column = &this->data[i];
        column->reserve(rows_);
        for (int j = i; j < rows_; j += cols_) {
            column->push_back(data_[j]);
        }
    }
    this->datetime_index.reserve(rows_);
    for(int i = 0 ; i < rows_; i++){
        this->datetime_index.push_back(datetime_index_[i]);
    }
    this->rows = rows_;
    this->cols = cols_;
    this->is_built = true;
}

double Asset::get(const std::string &column, size_t row_index) const {
    //fetch the column index of it exists
    size_t column_index;
    try {
        column_index = this->headers.at(column);
    } catch (const std::out_of_range& e) {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
    //make sure the row index is valid
    if(row_index >= this->rows){
        throw out_of_range("row index out of range");
    }
    return this->data[column_index][row_index];
}

Asset::Asset(string asset_id) :
        asset_id(std::move(asset_id)),
        is_built(false),
        rows(0),
        cols(0)
{}

std::shared_ptr<Asset> new_asset(const string& asset_id) {
    return std::make_shared<Asset>(asset_id);
}
