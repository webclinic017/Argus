#include <string>
#include <memory>
#include "../include/asset.h"
#include <pybind11/pybind11.h>

using namespace std;
namespace py = pybind11;

string Asset::get_asset_id() const{
    return this->asset_id;
}

bool Asset::get_is_built() const{
    return this->is_built;
}

void Asset::load_headers(const vector<std::string> &columns) {
    for (size_t i = 0; auto column_name : columns) {
        this->headers.emplace(column_name, i);
    }
}

std::shared_ptr<Asset> new_asset(const string& asset_id) {
    return std::make_shared<Asset>(asset_id);
}

PYBIND11_MODULE(asset, m) {
    m.doc() = "asset binding"; // optional module docstring
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id, "get the asset's id")
        .def("load_headers", &Asset::load_headers, "load asset headers");


    m.def("new_asset", &new_asset, py::return_value_policy::reference);
}