//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <string>
#include <memory>
#include <pybind11/pybind11.h>

#include "account.h"
#include "asset.h"
#include "broker.h"
#include "exchange.h"
#include "hydra.h"

namespace py = pybind11;
using namespace std;


void init_asset_ext(py::module &m) {
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id, "get the asset's id")
        .def("load_headers", &Asset::load_headers, "load asset headers")
        .def("load_data", &Asset::py_load_data, "load asset data")
        .def("get", &Asset::get, "get asset data")
        .def("get_datetime_index_view",
             &Asset::get_datetime_index_view,
             "get view of asset datetime index",
             py::return_value_policy::reference);

    m.def("new_asset", &new_asset, py::return_value_policy::reference);

    // Define a function that returns the memory address of a MyClass instance
    m.def("mem_address", [](Asset& instance) {
        return reinterpret_cast<std::uintptr_t>(&instance);
    });
}

void init_exchange_ext(py::module &m) {
    py::class_<Exchange, std::shared_ptr<Exchange>>(m, "Exchange")
        .def("build", &Exchange::build, "builds the exchange")
        .def("new_asset", &Exchange::new_asset, "builds a new asset to the exchange")
        .def("get_asset", &Exchange::get_asset, "get pointer to existing asset on the exchange", py::return_value_policy::reference)
        .def("register_asset", &Exchange::register_asset, "register a new asset to the exchange")
        .def("get_datetime_index_view", &Exchange::get_datetime_index_view, "get view of exchange datetime index");
}

void init_hydra_ext(py::module &m) {
    py::class_<Hydra, std::shared_ptr<Hydra>>(m, "Hydra")
        .def(py::init<int>())
        .def("new_exchange", &Hydra::new_exchange, "builds a new asset to the exchange",  py::return_value_policy::reference)
        .def("get_exchange", &Hydra::get_exchange, "builds a new asset to the exchange")
        .def("new_broker", &Hydra::new_broker, "builds a new broker object",  py::return_value_policy::reference)
        .def("get_broker", &Hydra::get_broker, "gets existing broker object");

    m.def("new_hydra", &new_hydra, py::return_value_policy::reference);
}

void init_account_ext(py::module &m) {
    py::class_<Account, std::shared_ptr<Account>>(m, "Account")
        .def(py::init<string,double>());
}

void init_broker_ext(py::module &m) {
    py::class_<Broker, std::shared_ptr<Broker>>(m, "Broker")
        .def("place_market_order", &Broker::place_market_order, "place market order")
        .def("place_limit_order", &Broker::place_limit_order, "place limit order");

}

PYBIND11_MODULE(FastTest, m) {
    m.doc() = "Argus bindings"; // optional module docstring

    //build python asset class bindings
    init_asset_ext(m);

    //built python exchange class bindings
    init_exchange_ext(m);

    //build python hydra class bindings
    init_hydra_ext(m);

    //build python account class bindings
    init_account_ext(m);

    //build python broker class bindings
    init_broker_ext(m);
}