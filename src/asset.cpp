#include <cstddef>
#include <iostream>
#include <string>
#include <memory>
#include <pybind11/stl.h>
#include <stdexcept>
#include "algorithm"

#include "asset.h"
#include "settings.h"
#include "utils_string.h"
#include "fmt/core.h"
#include "pybind11/numpy.h"

namespace py = pybind11;
using namespace std;


Asset::Asset(string asset_id_, string exchange_id_, string broker_id_, size_t warmup_)              
{
    this->asset_id = std::move(asset_id_);
    this->exchange_id = std::move(exchange_id_);
    this->broker_id = std::move(broker_id_);
    this->rows = 0,
    this->cols = 0,
    this->current_index = warmup_;
    this->warmup = warmup_;
    this->is_built = false;
}

Asset::~Asset()
{

#ifdef DEBUGGING
    printf("MEMORY:   CALLING ASSET %s DESTRUCTOR ON: %p \n", this->asset_id.c_str(), this);
#endif

    if (!this->is_built)
    {
        return;
    }
    if(this->is_view)
    {
        return;
    }

    // delete the underlying data
    delete[] this->data;

    // delete the datetime index
    delete[] this->datetime_index;

#ifdef DEBUGGING
    printf("MEMORY:   DESTRUCTOR ON: %p COMPLETE \n", this);
#endif
}

void Asset::reset_asset()
{   
    // move datetime index and data pointer back to start
    this->current_index = this->warmup;
    this->row = &this->data[this->warmup*this->cols];
}

string Asset::get_asset_id() const
{
    return this->asset_id;
}

bool Asset::get_is_built() const
{
    return this->is_built;
}

void Asset::load_headers(const vector<std::string> &columns)
{
    auto column_indecies = parse_headers(columns);
    this->open_column = std::get<0>(column_indecies);
    this->close_column = std::get<1>(column_indecies);

    size_t i = 0;
    for (const auto &column_name : columns)
    {
        this->headers.emplace(column_name, i);
        i++;
    }
}

void Asset::load_view(double *data_, long long *datetime_index_, size_t rows_, size_t cols_){
#ifdef DEBUGGING
    printf("MEMORY: CALLING ASSET %s load_data() ON: %p \n", this->asset_id.c_str(), this);
#endif  

    // allocate data array
    this->data = data_;

    // allocate datetime index
    this->datetime_index = new long long[rows_];

    // set the asset matrix size
    this->rows = rows_;
    this->cols = cols_;

    //is built and is a view
    this->is_view = true;
    this->is_built = true;

    this->row = &this->data[0];
#ifdef DEBUGGING
    printf("MEMORY:   asset %s datetime index at: %p \n", this->asset_id.c_str(), this->datetime_index);
    printf("MEMORY:   asset %s load_data() allocated at: %p  \n", this->asset_id.c_str(), this);
#endif
}

void Asset::load_data(const double *data_, const long long *datetime_index_, size_t rows_, size_t cols_)
{
#ifdef DEBUGGING
    printf("MEMORY: CALLING ASSET %s load_data() ON: %p \n", this->asset_id.c_str(), this);
#endif
    if (this->is_built)
    {
        throw runtime_error("asset is already built");
    }

    // allocate data array
    this->data = new double[rows_ * cols_];

    // allocate datetime index
    this->datetime_index = new long long[rows_];

    // set the asset matrix size
    this->rows = rows_;
    this->cols = cols_;

    // copy the data from a column formated 1d array ([col1_0, col1_1, col2_0, col2_1])
    for (int j = 0; j < cols_; j++) {
        auto input_col_start = j * rows_;
        for (int i = 0; i < rows_; i++) {
            data[i * cols_ + j] = data_[input_col_start + i];
        }
    }

    // copy the datetime index into the asset
    for (int i = 0; i < rows_; i++)
    {
        this->datetime_index[i] = datetime_index_[i];
    }

    //set row pointer to first row 
    this->row = &this->data[this->warmup * this->cols];

    // set build flag to true after copying data
    this->is_built = true;

#ifdef DEBUGGING
    printf("MEMORY:   asset %s datetime index at: %p \n", this->asset_id.c_str(), this->datetime_index);
    printf("MEMORY:   asset %s load_data() allocated at: %p  \n", this->asset_id.c_str(), this);
#endif
}

void Asset::py_load_data(
    const py::buffer &py_data,
    const py::buffer &py_datetime_index,
    size_t rows_,
    size_t cols_,
    bool is_view)
{
    py::buffer_info data_info = py_data.request();
    py::buffer_info datetime_index_info = py_datetime_index.request();

    // cast the python buffers to raw pointer
    auto data_ = static_cast<double *>(data_info.ptr);
    auto datetime_index_ = static_cast<long long *>(datetime_index_info.ptr);

    // pass raw pointer to c loading function and copy data
    if(!is_view)
    {
        this->load_data(data_, datetime_index_, rows_, cols_);
    }
    // pass raw pointer and mirror the asset pointer to the data passed in the py buffers
    else
    {
        this->load_view(data_, datetime_index_, rows_, cols_);
    }
}

double Asset::c_get(const std::string &column) const
{
    // fetch the column index of it exists
    size_t column_index;
    try
    {
        column_index = this->headers.at(column);
    }
    catch (const std::out_of_range &e)
    {
        throw std::runtime_error("failed to find column");
    }
    
    // derefence data pointer at current row plus column offset
    return *(this->row -this->cols + column_index);
}

double Asset::get(const std::string &column, size_t row_index) const
{
    // fetch the column index of it exists
    size_t column_index;
    try
    {
        column_index = this->headers.at(column);
    }
    catch (const std::out_of_range &e)
    {
        // Catch the exception and re-raise it as a Python KeyError
        throw py::key_error(e.what());
    }
    // make sure the row index is valid
    if (row_index >= this->rows)
    {
        throw out_of_range("row index out of range");
    }
    return this->data[row_index * this->cols + column_index];
}

double Asset::get_market_price(bool on_close) const
{

    #ifdef ARGUS_RUNTIME_ASSERT
    //make sure row pointer is not out of bounds
    ptrdiff_t index = this->row - this->data; 
    auto size = this->rows * this->cols;
    assert(index - this->cols  < size);
    #endif

    //subtract this->cols to move back row, then get_market_view is called, asset->step()
    //is called so we need to move back a row when accessing asset data
    if (on_close)
        return *(this->row - this->cols + this->close_column);
    else
        return *(this->row - this->cols + this->open_column);
}

double Asset::get_asset_feature(const string& column_name, int index)
{

    #ifdef ARGUS_RUNTIME_ASSERT
    //make sure row pointer is not out of bounds
    ptrdiff_t ptr_index = this->row - this->data; 
    auto size = this->rows * this->cols;
    assert(ptr_index - this->cols < size);
    assert(index <= 0);
    #endif

    //subtract this->cols to move back row, then get_market_view is called, asset->step()
    //is called so we need to move back a row when accessing asset data
    auto column_offset = this->headers.find(column_name);
    auto row_offset = static_cast<int>(this->cols) * index;

    #ifdef ARGUS_RUNTIME_ASSERT
    if(column_offset == this->headers.end()){
        throw std::runtime_error("failed to find column");
    }
    #endif

    //prevent acces index < 0
    assert(row_offset + ptr_index > 0);
    return *(this->row - this->cols + column_offset->second + row_offset);
}

py::array_t<double> Asset::get_column(const string& column_name, size_t length)
{
    if(length >= this->current_index)
    {
        throw std::runtime_error("index out of bounds");
    }

    auto column_offset = this->headers.find(column_name);
    auto row_offset = static_cast<int>(this->cols) * length;
    
    auto column_start = this->row - this->cols + column_offset->second - row_offset;
    return py::array( 
        py::buffer_info
            (
                column_start,                               /* Pointer to buffer */
                sizeof(double),                          /* Size of one scalar */
                py::format_descriptor<double>::format(), /* Python struct-style format descriptor */
                1,                                      /* Number of dimensions */
                { length },                 /* Buffer dimensions */
                { sizeof(double) * this->cols}
            )
    );
}

long long *Asset::get_datetime_index(bool warmup_start) const
{   
    if(warmup_start)
    {
        return &this->datetime_index[warmup];
    }
    else
    {
        return &this->datetime_index[0];
    }
}

py::array_t<long long> Asset::get_datetime_index_view()
{
    if (!this->is_built)
    {
        throw std::runtime_error("asset is not built");
    }
    if (this->rows == 0)
    {
        throw std::runtime_error("no data to return");
    }
    return to_py_array(
        this->datetime_index,
        this->rows,
        true);
};

long long *Asset::get_asset_time() const
{
    if (this->current_index == this->rows)
    {
        return nullptr;
    }
    else
    {
        return &this->datetime_index[this->current_index];
    }
}

void Asset::goto_datetime(long long datetime)
{
    //goto date is beyond the datetime index
   if(datetime >= this->datetime_index[this->rows-1])
    {
        this->current_index = this->rows;
    }
    
    // search for datetime in the index
    for(int i = this->current_index; i < this->rows; i++)
    {   
        //is >= right?
        if(this->datetime_index[i] >= datetime)
        {
            this->row += (this->cols * (i-this->warmup));
            this->current_index = i;
            return;
        }
    }

    throw std::runtime_error("failed to find datetime in asset goto");    
}

std::shared_ptr<Asset> new_asset(
    const string &asset_id,
    const string& exchange_id,
    const string& broker_id,
    size_t warmup)
{
    return std::make_shared<Asset>(asset_id, exchange_id, broker_id,warmup);
}

void Asset::step(){
    //move the row pointer forward to the next row
    this->row += this->cols;

    //move the current index forward
    this->current_index++; 

    for(auto& observer : this->asset_observers)
    {
        switch(observer.observer_type)
        {
            case AssetObserverType::Volatility:
                break;
            case AssetObserverType::Beta:
                break;
        }
    }
}