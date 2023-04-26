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
#include "order.h"

namespace py = pybind11;
using namespace std;

void init_asset_ext(py::module &m)
{
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id, "get the asset's id")
        .def("load_headers", &Asset::load_headers, "load asset headers")
        .def("load_data", &Asset::py_load_data, "load asset data")
        .def("get", &Asset::get, "get asset data")
        .def("get_mem_address", &Asset::get_mem_address, "get memory address")

        //.def("mem_address", []()
        .def("get_datetime_index_view",
             &Asset::get_datetime_index_view,
             "get view of asset datetime index",
             py::return_value_policy::reference);

    m.def("new_asset", &new_asset, py::return_value_policy::reference);

    // Define a function that returns the memory address of a MyClass instance
    m.def("mem_address", [](Asset &instance)
          { return reinterpret_cast<std::uintptr_t>(&instance); });
}

void init_exchange_ext(py::module &m)
{
    py::class_<Exchange, std::shared_ptr<Exchange>>(m, "Exchange")
        .def("build", &Exchange::build, "builds the exchange")
        .def("new_asset", &Exchange::new_asset, "builds a new asset to the exchange")
        .def("get_asset", &Exchange::get_asset, "get pointer to existing asset on the exchange", py::return_value_policy::reference)
        .def("register_asset", &Exchange::register_asset, "register a new asset to the exchange")
        .def("get_datetime_index_view", &Exchange::get_datetime_index_view, "get view of exchange datetime index");
}

void init_hydra_ext(py::module &m)
{
    py::class_<Hydra, std::shared_ptr<Hydra>>(m, "Hydra")
        .def(py::init<int>())
        .def("build", &Hydra::build, "build hydra class")

        .def("forward_pass", &Hydra::forward_pass, "forward pass")

        .def("new_exchange", &Hydra::new_exchange, "builds a new asset to the exchange", py::return_value_policy::reference)
        .def("new_broker", &Hydra::new_broker, "builds a new broker object", py::return_value_policy::reference)
        .def("new_portfolio", &Hydra::new_portfolio, "adds new portfolio to master portfolio", py::return_value_policy::reference)
        
        .def("get_broker", &Hydra::get_broker, "gets existing broker object")
        .def("get_master_portfolio", &Hydra::get_master_portflio, "get smart pointer to master portfolio")
        .def("get_exchange", &Hydra::get_exchange, "builds a new asset to the exchange");

    m.def("new_hydra", &new_hydra, py::return_value_policy::reference);
}

void init_account_ext(py::module &m)
{
    py::class_<Account, std::shared_ptr<Account>>(m, "Account")
        .def(py::init<string, double>());
}

void init_portfolio_ext(py::module &m)
{
    py::class_<Portfolio, std::shared_ptr<Portfolio>>(m, "Portfolio")
        .def("get_mem_address", &Portfolio::get_mem_address, "get memory address")
        .def("get_portfolio_id", &Portfolio::get_portfolio_id, "get portfolio id")

        .def("place_market_order", &Portfolio::place_market_order, "place market order")
        
        .def("create_sub_portfolio", &Portfolio::create_sub_portfolio, "create new child portfolio")
        .def("find_portfolio", &Portfolio::find_portfolio, "find a child portfolio");

}

void init_broker_ext(py::module &m)
{
    py::class_<Broker, std::shared_ptr<Broker>>(m, "Broker");

}

void init_enum(py::module &m){
     py::enum_<OrderExecutionType>(m, "OrderExecutionType")
        .value("EAGER", OrderExecutionType::EAGER)
        .value("Green", OrderExecutionType::LAZY)
        .export_values();
}

PYBIND11_MODULE(FastTest, m)
{
    m.doc() = "Argus bindings"; // optional module docstring

    // build python asset class bindings
    init_asset_ext(m);

    // built python exchange class bindings
    init_exchange_ext(m);

    // build python hydra class bindings
    init_hydra_ext(m);

    // build python account class bindings
    init_account_ext(m);

    // build python broker class bindings
    init_broker_ext(m);

    // build python portfolio class bindings
    init_portfolio_ext(m);

    // build python enum bindings
    init_enum(m);
}