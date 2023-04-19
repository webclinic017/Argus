#include <iostream>
#include <string>
#include <memory>
#include <pybind11/stl.h>
#include <stdexcept>

#include "../include/asset.h"
#include "../include/settings.h"

namespace py = pybind11;
using namespace std;

string Asset::get_asset_id() const{
    return this->asset_id;
}

bool Asset::get_is_built() const{
    return this->is_built;
}

void Asset::load_headers(const vector<std::string> &columns) {
    size_t i = 0;
    for (const auto& column_name : columns) {
        this->headers.emplace(column_name, i);
        i++;
    }
}

void Asset::load_data(const double *data_, const long long *datetime_index_, size_t rows_, size_t cols_) {
#ifdef DEBUGGING
    printf("MEMORY: CALLING ASSET %s load_data() ON: %p \n",this->asset_id.c_str(), this);
#endif
    if (this->is_built){
        throw runtime_error("asset is already built");
    }

    // allocate columns
    this->data = new double*[cols_];
    for (int i = 0; i < cols_; i++) {
        // allocate space for individual columns
        this->data[i] = new double[rows_];
    }
    //allocate datetime index
    this->datetime_index = new long long[rows_];

    //set the asset matrix size
    this->rows = rows_;
    this->cols = cols_;

    for (int i = 0; i < cols_; i++) {
        auto column = this->data[i];
        size_t offset = i * rows;
        size_t row_index = 0;
        for (size_t j = offset; j < offset + rows_; j++) {
            column[row_index] = data_[j];
            row_index ++;
        }
    }

    //copy the datetime index into the asset
    for(int i = 0 ; i < rows_; i++){
        this->datetime_index[i] = datetime_index_[i];
    }
    this->is_built = true;

#ifdef DEBUGGING
    printf("MEMORY: ASSET %s load_data(): %p ALLOCATED \n",this->asset_id.c_str(), this);
#endif
}

void Asset::py_load_data(const py::buffer& py_data, const py::buffer&  py_datetime_index, size_t rows_, size_t cols_) {
    py::buffer_info data_info = py_data.request();
    py::buffer_info datetime_index_info = py_datetime_index.request();

    //cast the python buffers to raw pointer
    auto data_ = static_cast<const double*>(data_info.ptr);
    auto datetime_index_ = static_cast<const long long *>(datetime_index_info.ptr);

    //pass raw pointer to c loading function
    this->load_data(data_, datetime_index_, rows_, cols_);
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
        is_view(false),
        rows(0),
        cols(0),
        current_index(0)
{}

Asset::~Asset() {

#ifdef DEBUGGING
    printf("MEMORY: CALLING ASSET %s DESTRUCTOR ON: %p \n",this->asset_id.c_str(), this);
#endif

    if (!this->is_built){
        return;
    }
    //free each column data
    for (int i = 0; i < this->cols; i++) {
        delete[] this->data[i];
    }
    //free the data itself
    delete[] this->data;

    //free the datetime index
    delete[] this->datetime_index;

#ifdef DEBUGGING
    printf("MEMORY: DESTRUCTOR ON: %p COMPLETE \n", this);
#endif
}

std::shared_ptr<Asset> new_asset(const string& asset_id) {
    return std::make_shared<Asset>(asset_id);
}
