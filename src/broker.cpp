//
// Created by Nathan Tormaschy on 4/21/23.
//
#include <cstdio>
#include <string>
#include <memory>
#include <fmt/core.h>

#include "broker.h"
#include "position.h"
#include "account.h"
#include "settings.h"
#include "utils_array.h"
#include "utils_time.h"

using namespace std;


Broker::Broker(string broker_id_,
               double cash_,
               int logging_,
               shared_ptr<History> history_,
               shared_ptr<Portfolio> master_portfolio) : broker_account(broker_id_, cash_)
{
    this->history = std::move(history_);
    this->master_portfolio = std::move(master_portfolio);

    this->broker_id = std::move(broker_id_);
    this->cash = cash_;
    this->logging = logging_;
    this->order_counter = 0;
    this->position_counter = 0;
}

void Broker::build(
    Exchanges *exchanges_)
{
    this->exchanges = exchanges_;
}

void Broker::cancel_order(unsigned int order_id)
{
    auto order = unsorted_vector_remove(
        this->open_orders,
        [](const shared_ptr<Order> &obj)
        { return obj->get_order_id(); },
        order_id);

    // set the order state to cancel
    order->set_order_state(CANCELED);

    // get the order's parent if exists
    auto order_parent_struct = order->get_order_parent();

    // if the order has no parent then return
    if (!order_parent_struct)
    {
        // move canceled order to history
        this->history->remember_order(std::move(order));
        return;
    }

    // remove the open order from the open order's parent
    switch (order_parent_struct->order_parent_type)
    {
    case TRADE:
    {
        auto trade = order_parent_struct->member.parent_trade;
        trade->cancel_child_order(order_id);
    }
    case ORDER:
    {
        auto parent_order = order_parent_struct->member.parent_order;
        parent_order->cancel_child_order(order_id);
    }
    }

    // recursively cancel all child orders of the canceled order
    for (auto &child_orders : order->get_child_orders())
    {
        this->cancel_order(child_orders->get_order_id());
    }

    // move canceled order to history
    this->history->remember_order(std::move(order));
}

void Broker::place_order_buffer(shared_ptr<Order> order)
{
    this->open_orders_buffer.push_back(order);
}

void Broker::place_order(shared_ptr<Order> order)
{
    // get smart pointer to the right exchange
    auto exchange = this->exchanges->at(order->get_exchange_id());

    // send the order
    exchange->place_order(order);

    // if the order was filled then process fill
    if (order->get_order_state() == FILLED)
    {
        this->process_filled_order(std::move(order));
    }
    // else push the order to the open order vector to be monitored
    else
    {
        this->open_orders.push_back(order);
    }
}

void Broker::send_orders()
{
    // send orders from buffer to the exchange
    for (auto &order : this->open_orders_buffer)
    {
        // get the exchange the order was placed to
        auto exchange = exchanges->at(order->get_exchange_id());

        // send order to rest on the exchange
        exchange->place_order(order);

        if (order->get_order_state() == FILLED)
        {
            // process order that has been filled
            this->process_filled_order(order);
        }
        else
        {
            // add the order to current open orders
            this->open_orders.push_back(order);
        }
    }
    // clear the order buffer as all orders are now open
    this->open_orders_buffer.clear();
}

void Broker::process_filled_order(order_sp_t filled_order)
{
    // adjust the account held at the broker
    assert(filled_order->get_source_portfolio());
    this->broker_account.on_order_fill(filled_order);

    // get the portfolio the order was placed for, adjust the sub portfolio accorindly
    filled_order->get_source_portfolio()->on_order_fill(filled_order);

    //remember the order
    this->history->remember_order(std::move(filled_order));
}

void Broker::process_orders()
{
    // iterate over open orders and process and filled ones
    for (auto it = this->open_orders.begin(); it != this->open_orders.end();)
    {
        auto order = *it;

        if (order->get_order_state() != FILLED)
        {
            it++;
            continue;
        }
        else
        {
            // process the individual order
            this->process_filled_order(std::move(order));

            // remove filled order and move to next open order
            it = this->open_orders.erase(it);
        }
    }
};
