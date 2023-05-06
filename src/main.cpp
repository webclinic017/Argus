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
#include "pybind11/cast.h"
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
        .def("get_column", &Asset::get_column)

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
            py::arg("column_name"),
            py::arg("row") = 0,
            py::arg("query_type") = ExchangeQueryType::Default,
            py::arg("N") = -1)

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
        .def("run", &Hydra::run,
            py::arg("steps") = 0,
            py::arg("to") = 0)
        .def("reset_strategies", &Hydra::reset_strategies)
        .def("reset", &Hydra::reset,
            py::arg("clear_history") = true,
            py::arg("clear_strategies") = false)
        .def("replay", &Hydra::replay)
        .def("goto_datetime", &Hydra::goto_datetime)

        #ifdef ARGUS_STRIP
        .def("forward_pass", &Hydra::forward_pass)
        .def("on_open", &Hydra::on_open)
        .def("backward_pass", &Hydra::backward_pass)
        #endif

        .def("new_strategy", &Hydra::new_strategy)
        .def("new_exchange", &Hydra::new_exchange, py::return_value_policy::reference)
        .def("new_broker", &Hydra::new_broker, py::return_value_policy::reference)
        .def("new_portfolio", &Hydra::new_portfolio, py::return_value_policy::reference)
        
        .def("get_hydra_time", &Hydra::get_hydra_time)
        .def("get_datetime_index_view", &Hydra::get_datetime_index_view)
        .def("get_order_history", &Hydra::get_order_history)
        .def("get_candles", &Hydra::get_candles)
        .def("get_broker", &Hydra::get_broker)
        .def("get_master_portfolio", &Hydra::get_master_portflio)
        .def("get_portfolio", &Hydra::get_portfolio)
        .def("get_exchange", &Hydra::get_exchange);

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

        .def("add_tracer", &Portfolio::add_tracer, py::return_value_policy::reference)
        .def("get_tracer", &Portfolio::get_tracer, py::return_value_policy::reference)

        .def("get_nlv", &Portfolio::get_nlv)
        .def("get_cash", &Portfolio::get_cash)
        .def("get_unrealized_pl", &Portfolio::get_unrealized_pl)

        .def("close_position", &Portfolio::py_close_position,
            py::arg("asset_id") = "")
        .def("place_market_order", &Portfolio::place_market_order,
            py::arg("asset_id"),
            py::arg("units"),
            py::arg("strategy_id"),
            py::arg("order_execution_type") = OrderExecutionType::LAZY,
            py::arg("trade_id") = -1)
        .def("order_target_allocations",&Portfolio::order_target_allocations,
            py::arg("allocations"),
            py::arg("strategy_id"),
            py::arg("epsilon"),
            py::arg("order_execution_type") = OrderExecutionType::LAZY,
            py::arg("order_target_type") = OrderTargetType::PCT,
            py::arg("clear_missing") = true)
        .def("order_target_size", &Portfolio::order_target_size)
        
        .def("create_sub_portfolio", &Portfolio::create_sub_portfolio)
        .def("find_portfolio", &Portfolio::find_portfolio);

    py::class_<PortfolioTracer, shared_ptr<PortfolioTracer>>(m, "PortfolioTracer");
    py::class_<ValueTracer, PortfolioTracer, shared_ptr<ValueTracer>>(m, "ValueTracer")
        .def("get_nlv_history", &ValueTracer::get_nlv_history)
        .def("get_cash_history", &ValueTracer::get_cash_history);

    py::class_<EventTracer, PortfolioTracer, shared_ptr<EventTracer>>(m, "EventTracer")
        .def("get_order_history",&EventTracer::get_order_history,
            py::return_value_policy::reference_internal
        )
        .def("get_trade_history",&EventTracer::get_trade_history,
            py::return_value_policy::reference_internal
        )
        .def("get_position_history",&EventTracer::get_position_history,
            py::return_value_policy::reference_internal
        );
}

void init_position_ext(py::module &m)
{
    py::class_<Position, std::shared_ptr<Position>>(m, "Position")
        .def("get_trade", &Position::get_trade)
        
        .def("get_average_price", &Position::get_average_price)
        .def("get_close_price", &Position::get_close_price)
        .def("get_last_price", &Position::get_last_price)
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

    py::enum_<PortfolioTracerType>(m, "PortfolioTracerType")
        .value("VALUE", PortfolioTracerType::Value)
        .value("EVENT", PortfolioTracerType::Event)
        .export_values();

    py::enum_<ExchangeQueryType>(m, "ExchangeQueryType")
        .value("DEFAULT", ExchangeQueryType::Default)
        .value("NLARGEST", ExchangeQueryType::NLargest)
        .value("NSMALLEST", ExchangeQueryType::NSmallest)
        .value("NEXTREME", ExchangeQueryType::NExtreme)
        .export_values();       
}

PYBIND11_MODULE(FastTest, m)
{
    m.doc() = "Argus bindings"; // optional module docstring

    // build python enum bindings
    init_enum(m);

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
}