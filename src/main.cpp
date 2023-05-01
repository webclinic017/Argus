//
// Created by Nathan Tormaschy on 4/18/23.
//
#include <cstddef>
#include <fmt/core.h>
#include <string>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include "account.h"
#include "asset.h"
#include "broker.h"
#include "exchange.h"
#include "hydra.h"
#include "order.h"
#include "portfolio.h"
#include "position.h"
#include "settings.h"

namespace py = pybind11;
using namespace std;

void init_asset_ext(py::module &m)
{
    py::class_<Asset, std::shared_ptr<Asset>>(m, "Asset")
        .def("get_asset_id", &Asset::get_asset_id)
        .def("load_headers", &Asset::load_headers)
        .def("load_data", &Asset::py_load_data)
        .def("get", &Asset::get)
        .def("get_mem_address", &Asset::get_mem_address)

        //.def("mem_address", []()
        .def("get_datetime_index_view",
             &Asset::get_datetime_index_view,
             py::return_value_policy::reference);

    m.def("new_asset", &new_asset, py::return_value_policy::reference,
            py::arg("asset_id"),
            py::arg("exchange_id"),
            py::arg("broker_id"),
            py::arg("warmup") = 0
    );

    // Define a function that returns the memory address of a MyClass instance
    m.def("mem_address", [](Asset &instance)
          { return reinterpret_cast<std::uintptr_t>(&instance); });
}

void init_exchange_ext(py::module &m)
{
    py::class_<Exchange, std::shared_ptr<Exchange>>(m, "Exchange")
        .def("build", &Exchange::build)
        .def("new_asset", &Exchange::new_asset)
        .def("register_asset", &Exchange::register_asset)
        
        .def("get_asset", &Exchange::get_asset, py::return_value_policy::reference)
        .def("get_exchange_feature", 
            &Exchange::get_exchange_feature, 
            py::arg("feature_dict"),
            py::arg("column_name"),
            py::arg("row") = 0)

        .def("get_asset_feature", 
            &Exchange::get_asset_feature, 
            py::arg("asset_id"),
            py::arg("column_name"),
            py::arg("index") = 0)

        .def("get_datetime_index_view", &Exchange::get_datetime_index_view);
}

void init_hydra_ext(py::module &m)
{
    py::class_<Hydra, std::shared_ptr<Hydra>>(m, "Hydra")
        .def(py::init<int,double>())
        .def("get_void_ptr", [](Hydra& self) {
                    void* ptr = self.void_ptr();
                    return py::capsule(ptr, "void*");
                })
        .def("build", &Hydra::build)
        .def("run", &Hydra::run)
        .def("reset", &Hydra::reset)
        .def("replay", &Hydra::replay)

        #ifdef ARGUS_STRIP
        .def("forward_pass", &Hydra::forward_pass)
        .def("on_open", &Hydra::on_open)
        .def("backward_pass", &Hydra::backward_pass)
        #endif

        .def("new_strategy", &Hydra::new_strategy)
        .def("new_exchange", &Hydra::new_exchange, py::return_value_policy::reference)
        .def("new_broker", &Hydra::new_broker, py::return_value_policy::reference)
        .def("new_portfolio", &Hydra::new_portfolio, py::return_value_policy::reference)
        
        .def("get_candles", &Hydra::get_candles)
        .def("get_broker", &Hydra::get_broker)
        .def("get_master_portfolio", &Hydra::get_master_portflio)
        .def("get_portfolio", &Hydra::get_portfolio)
        .def("get_exchange", &Hydra::get_exchange)

        .def("get_order_history", [](Hydra& self) {
            return self.get_history()->get_order_history();},
            py::return_value_policy::reference_internal
        )
        .def("get_trade_history", [](Hydra& self) {
            return self.get_history()->get_trade_history();},
            py::return_value_policy::reference_internal
        )
        .def("get_position_history", [](Hydra& self) {
            return self.get_history()->get_position_history();},
            py::return_value_policy::reference_internal
        );

    m.def("new_hydra", &new_hydra, py::return_value_policy::reference);
}

void init_strategy_ext(py::module &m)
{
    py::class_<Strategy, std::shared_ptr<Strategy>>(m, "Strategy")
        .def_readwrite("on_close", &Strategy::python_handler_on_close)
        .def_readwrite("on_open", &Strategy::python_handler_on_open);
}

void init_account_ext(py::module &m)
{
    py::class_<Account, std::shared_ptr<Account>>(m, "Account")
        .def(py::init<string, double>());
}

void init_portfolio_ext(py::module &m)
{
    py::class_<Portfolio, std::shared_ptr<Portfolio>>(m, "Portfolio")
        .def("get_mem_address", &Portfolio::get_mem_address)
        .def("get_portfolio_id", &Portfolio::get_portfolio_id)
        .def("get_position", &Portfolio::get_position)
        .def("get_portfolio_history", &Portfolio::get_portfolio_history)

        .def("get_nlv", &Portfolio::get_nlv)
        .def("get_cash", &Portfolio::get_cash)
        .def("get_unrealized_pl", &Portfolio::get_unrealized_pl)
        
        .def("place_market_order", &Portfolio::place_market_order)
        .def("order_target_size", &Portfolio::order_target_size)
        
        .def("create_sub_portfolio", &Portfolio::create_sub_portfolio)
        .def("find_portfolio", &Portfolio::find_portfolio);

    py::class_<PortfolioHistory, std::shared_ptr<PortfolioHistory>>(m, "PortfolioHistory")
        .def("get_cash_history", &PortfolioHistory::get_cash_history,py::return_value_policy::reference)
        .def("get_nlv_history", &PortfolioHistory::get_nlv_history,py::return_value_policy::reference);

}

void init_position_ext(py::module &m)
{
    py::class_<Position, std::shared_ptr<Position>>(m, "Position")
        .def("get_trade", &Position::get_trade)
        
        .def("get_average_price", &Position::get_average_price)
        .def("get_close_price", &Position::get_close_price)
        .def("get_units", &Position::get_units)
        .def("get_nlv", &Position::get_nlv)
        .def("get_unrealized_pl", &Position::get_unrealized_pl)
        .def("get_position_close_time", &Position::get_position_close_time)
        .def("get_position_open_time", &Position::get_position_open_time)
        .def("get_asset_id", &Position::get_asset_id)
        .def("get_exchange_id", &Position::get_exchange_id)
        .def("get_position_id", &Position::get_position_id)

        .def_readonly("is_open", &Position::is_open)
        .def_readonly("units", &Position::units)
        .def_readonly("average_price", &Position::average_price);

    py::class_<Trade, std::shared_ptr<Trade>>(m, "Trade")
        .def("get_mem_address", &Trade::get_mem_address)
        .def("get_average_price", &Trade::get_average_price)
        .def("get_units", &Trade::get_units);
}

void init_broker_ext(py::module &m)
{
    py::class_<Broker, std::shared_ptr<Broker>>(m, "Broker");

    py::class_<Order, std::shared_ptr<Order>>(m, "Order")
        .def("get_order_type", &Order::get_order_type)
        .def("get_order_state", &Order::get_order_state)
        .def("get_order_id", &Order::get_order_id)
        .def("get_trade_id", &Order::get_trade_id)
        .def("get_units", &Order::get_units)
        .def("get_average_price", &Order::get_average_price)
        .def("get_fill_time", &Order::get_fill_time)
        .def("get_limit", &Order::get_limit)
        .def("get_asset_id", &Order::get_asset_id)
        .def("get_exchange_id", &Order::get_exchange_id)
        .def("get_broker_id", &Order::get_broker_id)
        .def("get_strategy_id", &Order::get_strategy_id)
        .def("get_portfolio_id", [](Order& self) {
            return self.get_source_portfolio()->get_portfolio_id();
        }
        );

}

void init_enum(py::module &m){
     py::enum_<OrderExecutionType>(m, "OrderExecutionType")
        .value("EAGER", OrderExecutionType::EAGER)
        .value("LAZY", OrderExecutionType::LAZY)
        .export_values();

    py::enum_<OrderType>(m, "OrderType")
        .value("MARKET_ORDER", OrderType::MARKET_ORDER)
        .value("LIMIT_ORDER", OrderType::LIMIT_ORDER)
        .value("STOP_LOSS_ORDER", OrderType::STOP_LOSS_ORDER)
        .value("TAKE_PROFIT_ORDER", OrderType::TAKE_PROFIT_ORDER)
        .export_values();

    py::enum_<OrderState>(m, "OrderState")
        .value("PENDING", OrderState::PENDING)
        .value("OPEN", OrderState::OPEN)
        .value("FILLED", OrderState::FILLED)
        .value("CANCELED", OrderState::CANCELED)
        .export_values();

    py::enum_<OrderTargetType>(m, "OrderTargetType")
        .value("UNITS", OrderTargetType::UNITS)
        .value("DOLLARS", OrderTargetType::DOLLARS)
        .value("PCT", OrderTargetType::PCT)
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

    // build python strategy class bindings
    init_strategy_ext(m);

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