#include <iostream>
#include <string>
#include <memory>
#include <pybind11/stl.h>
#include <stdexcept>

#include "../include/asset.h"
#include "../include/settings.h"
#include "../include/utils_string.h"

namespace py = pybind11;
using namespace std;

string Asset::get_asset_id() const{
    return this->asset_id;
}

bool Asset::get_is_built() const{
    return this->is_built;
}

void Asset::load_headers(const vector<std::string> &columns) {
    auto column_indecies = parse_headers(columns);
    this->open_column = get<0>(column_indecies);
    this->close_column = get<1>(column_indecies);
    
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
    printf("MEMORY:   asset %s datetime index at: %p \n",this->asset_id.c_str(), this->datetime_index);
    printf("MEMORY:   asset %s load_data() allocated at: %p  \n",this->asset_id.c_str(), this);
#endif
}

void Asset::load_data_view(double *data_, long long *datetime_index_, size_t rows_, size_t cols_) {
    if (this->is_built){
        throw runtime_error("asset is already built");
    }

    //allocate columns, underlying data is in row major format. Point the start of each column
    //to the appropriate index in the underlying data
    this->data = new double*[cols_];
    for (int i = 0; i < cols_; i++) {
        size_t offset = i * rows_;
        this->data[i] = &data_[offset];
    }
    this->rows = rows_;
    this->cols = cols_;

    //allocate datetime index
    this->datetime_index = &datetime_index_[0];

    this->is_built = true;
    this->is_view = true;
}

void Asset::py_load_data(const py::buffer& py_data, const py::buffer&  py_datetime_index, size_t rows_, size_t cols_, bool is_view) {
    py::buffer_info data_info = py_data.request();
    py::buffer_info datetime_index_info = py_datetime_index.request();

    //cast the python buffers to raw pointer
    auto data_ = static_cast<double*>(data_info.ptr);
    auto datetime_index_ = static_cast<long long *>(datetime_index_info.ptr);

    //pass raw pointer to c loading function
    if(!is_view){
        this->load_data(data_, datetime_index_, rows_, cols_);
    } else{
        this->load_data_view(data_, datetime_index_, rows_, cols_);
    }

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

long long *Asset::get_datetime_index() const {
    return &this->datetime_index[0];
}

py::array_t<long long> Asset::get_datetime_index_view() {
    if(!this->is_built){
        throw std::runtime_error("asset is not built");
    }
    if(this->rows == 0){
        throw std::runtime_error("no data to return");
    }
    return to_py_array(
            this->datetime_index,
            this->rows,
            true);
};

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
    printf("MEMORY:   CALLING ASSET %s DESTRUCTOR ON: %p \n",this->asset_id.c_str(), this);
#endif

    if (!this->is_built){
        return;
    }

    if(!this->is_view) {
        //free each column data
        for (int i = 0; i < this->cols; i++) {
            delete[] this->data[i];
        }
    }
    //free the data itself
    delete[] this->data;

    //free the datetime index
    if (!this->is_view) {
        delete[] this->datetime_index;
    }

#ifdef DEBUGGING
    printf("MEMORY:   DESTRUCTOR ON: %p COMPLETE \n", this);
#endif
}

long long* Asset::get_asset_time() const {
    if(this->current_index == this->rows){
        return nullptr;
    }
    else {
        return &this->datetime_index[this->current_index];
    }
}

std::shared_ptr<Asset> new_asset(const string& asset_id) {
    return std::make_shared<Asset>(asset_id);
}

