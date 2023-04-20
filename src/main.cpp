//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <string>
#include <memory>
#include <pybind11/pybind11.h>

#include "asset.h"
#include "exchange.h"
#include "hydra.h"

namespace py = pybind11;


void init_asset_ext(py::module &m) {
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id, "get the asset's id")
        .def("load_headers", &Asset::load_headers, "load asset headers")
        .def("load_data", &Asset::py_load_data, "load asset data")
        .def("get", &Asset::get, "get asset data");

    m.def("new_asset", &new_asset, py::return_value_policy::reference);

    // Define a function that returns the memory address of a MyClass instance
    m.def("mem_address", [](Asset& instance) {
        return reinterpret_cast<std::uintptr_t>(&instance);
    });
}

void init_exchange_ext(py::module &m) {
    py::class_<Exchange, std::shared_ptr<Exchange>>(m, "Exchange")
        .def("new_asset", &Exchange::new_asset, "builds a new asset to the exchange")
        .def("get_asset", &Exchange::get_asset, "get pointer to existing asset on the exchange")
        .def("register_asset", &Exchange::register_asset, "register a new asset to the exchange");
}

void init_hydra_ext(py::module &m) {
    py::class_<Hydra, std::shared_ptr<Hydra>>(m, "Hydra")
        .def("new_exchange", &Hydra::new_exchange, "builds a new asset to the exchange")
        .def("get_exchange", &Hydra::get_exchange, "builds a new asset to the exchange");

    m.def("new_hydra", &new_hydra, py::return_value_policy::reference);
}

PYBIND11_MODULE(FastTest, m) {
    m.doc() = "Argus bindings"; // optional module docstring
    init_asset_ext(m);
    init_exchange_ext(m);
    init_hydra_ext(m);
}