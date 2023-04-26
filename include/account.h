#ifndef ARGUS_ACCOUNT_H
#define ARGUS_ACCOUNT_H

#include <memory>
#include <string>
#include <tsl/robin_map.h>

#include "trade.h"
#include "order.h"

class Account
{
public:
    using order_sp_t = Order::order_sp_t;

    /// @brief account constructor
    /// @param account_id
    Account(std::string account_id, double cash);

    /// @brief unique id of the account
    std::string account_id;

    /// @brief cash held by the portfolio
    double cash;

    /// @brief process a filled order for the account
    /// @param filled_order reference to a smart pointer for a filled order
    void on_order_fill(order_sp_t filled_order);

private:
    /// @brief map between asset ids and trades
    tsl::robin_map<std::string, Trade> trades;
};

#endif