#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include "fmt/core.h"

#include "trade.h"
#include "account.h"

using order_sp_t = Order::order_sp_t;

Account::Account(std::string account_id_, double cash_) : trades()
{
    this->account_id = account_id_;
    this->cash = cash_;
}

void Account::on_order_fill(order_sp_t filled_order)
{   
    // get order information
    auto asset_id = filled_order->get_asset_id();
    auto order_units = filled_order->get_units();
    auto order_fill_price = filled_order->get_average_price();

    // adjust the account's cash
    this->cash -= order_units * order_fill_price;

    // find trade if exists
    auto it = this->trades.find(asset_id);
    
    // no trade exists with the given asset id
    if (it == this->trades.end())
    {
        this->trades.insert({asset_id,
                             Trade(
                                filled_order, true
                                )});
    }
    // trade exists, modify it accorind to the order
    else
    {
        auto trade = &this->trades.at(asset_id);

        auto trade_units = trade->get_units();
        if(trade_units * order_units < 0 && abs(order_units) > abs(trade_units))
        {
            // new order created to close out existing position, filled order now holds units 
            // to open the position in the other direction
            auto new_order = split_order(
                filled_order,  
                -1 * trade_units);

            // process adjusted orders, first closes out, second creates new position
            this->on_order_fill(new_order);
            this->on_order_fill(filled_order);
            return;
        }
        trade->adjust(filled_order);

        if(!trade->get_is_open())
        {
            this->trades.erase(asset_id);
            assert(!this->trades.contains(asset_id));
        }
    }
};