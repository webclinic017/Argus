//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <fmt/core.h>
#include <string>
#include <memory>
#include <pybind11/pybind11.h>

#include "account.h"
#include "asset.h"
#include "broker.h"
#include "exchange.h"
#include "hydra.h"
#include "order.h"
#include "portfolio.h"
#include "position.h"

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
        .def("register_asset", &Exchange::register_asset, "register a new asset to the exchange")
        
        .def("get_asset", &Exchange::get_asset, "get pointer to existing asset on the exchange", py::return_value_policy::reference)
        .def("get_asset_feature", 
            &Exchange::get_asset_feature, 
            "get asset feature",
            py::arg("asset_id"),
            py::arg("column_name"),
            py::arg("index") = 0)

        .def("get_datetime_index_view", &Exchange::get_datetime_index_view, "get view of exchange datetime index");
}

void init_hydra_ext(py::module &m)
{
    py::class_<Hydra, std::shared_ptr<Hydra>>(m, "Hydra")
        .def(py::init<int>())
        .def("get_void_ptr", [](Hydra& self) {
                    void* ptr = self.void_ptr();
                    return py::capsule(ptr, "void*");
                })
        .def("build", &Hydra::build, "build hydra class")

        .def("forward_pass", &Hydra::forward_pass, "forward pass")
        .def("on_open", &Hydra::on_open, "on open")
        .def("backward_pass", &Hydra::backward_pass, "backwards pass")

        .def("new_exchange", &Hydra::new_exchange, "builds a new asset to the exchange", py::return_value_policy::reference)
        .def("new_broker", &Hydra::new_broker, "builds a new broker object", py::return_value_policy::reference)
        .def("new_portfolio", &Hydra::new_portfolio, "adds new portfolio to master portfolio", py::return_value_policy::reference)
        
        .def("get_broker", &Hydra::get_broker, "gets existing broker object")
        .def("get_master_portfolio", &Hydra::get_master_portflio, "get smart pointer to master portfolio")
        .def("get_portfolio", &Hydra::get_portfolio, "search through portfolio tree to find portfolio")
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
        .def("get_position", &Portfolio::get_position, "get position from portfolio")
        .def("get_portfolio_history", &Portfolio::get_portfolio_history, "get portfolio history object")

        .def("get_nlv", &Portfolio::get_nlv, "get net liquidation value")
        .def("get_cash", &Portfolio::get_cash, "get cash held in the portfolio")
        .def("get_unrealized_pl", &Portfolio::get_unrealized_pl, "get unrealized pl")
        
        .def("place_market_order", &Portfolio::place_market_order, "place market order")
        
        .def("create_sub_portfolio", &Portfolio::create_sub_portfolio, "create new child portfolio")
        .def("find_portfolio", &Portfolio::find_portfolio, "find a child portfolio");

    py::class_<PortfolioHistory, std::shared_ptr<PortfolioHistory>>(m, "PortfolioHistory")
        .def("get_cash_history", &PortfolioHistory::get_cash_history, "get cash history",py::return_value_policy::reference)
        .def("get_nlv_history", &PortfolioHistory::get_nlv_history, "get cash history",py::return_value_policy::reference);

}

void init_position_ext(py::module &m)
{
    py::class_<Position, std::shared_ptr<Position>>(m, "Position")
        .def("get_trade", &Position::get_trade, "get child trade from position")
        
        .def("get_average_price", &Position::get_average_price, "get position average price")
        .def("get_units", &Position::get_units, "get position units")
        .def("get_nlv", &Position::get_nlv, "get net liquidation value")
        .def("get_unrealized_pl", &Position::get_unrealized_pl, "get unrealized pl")
        
        .def("is_open", &Position::get_is_open, "is position open");

    py::class_<Trade, std::shared_ptr<Trade>>(m, "Trade")
        .def("get_mem_address", &Trade::get_mem_address, "get memory address of trade object")
        .def("get_average_price", &Trade::get_average_price, "get trade average price")
        .def("get_units", &Trade::get_units, "get trade units");
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

    // build python position class bindings
    init_position_ext(m);

    // build python enum bindings
    init_enum(m);
}