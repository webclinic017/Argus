//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <string>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "asset.h"

namespace py = pybind11;


void init_asset_ext(py::module &m) {
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id, "get the asset's id")
        .def("load_headers", &Asset::load_headers, "load asset headers")
        .def("load_data", &Asset::py_load_data, "load asset data")
        .def("get", &Asset::get, "get asset data");



m.def("new_asset", &new_asset, py::return_value_policy::reference);
}


PYBIND11_MODULE(FastTest, m) {
    m.doc() = "Argus bindings"; // optional module docstring
    init_asset_ext(m);
}