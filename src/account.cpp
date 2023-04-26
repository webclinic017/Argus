#include <cstddef>
#include <memory>
#include <string>
#include <tsl/robin_map.h>

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

    
    // no trade exists with the given asset id
    if (!this->trades.contains(asset_id))
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
        trade->adjust(filled_order);

        if(!trade->get_is_open())
        {
            this->trades.erase(asset_id);
        }
    }
};