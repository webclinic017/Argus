//
// Created by Nathan Tormaschy on 4/21/23.
//
#include <string>
#include <utility>
#include "account.h"
#include "settings.h"

Account::Account(string account_id_, double starting_cash) {
    this->account_id = std::move(account_id_);
    this->starting_cash = starting_cash;
    this->cash = starting_cash;
    this->nlv = starting_cash;
    this->unrealized_pl = 0.0;
    this->realized_pl = 0.0;
    this->portfolio = {};
}

void Account::new_trade(const shared_ptr<Trade>& trade) {
    //adjust the accounts available cash
    this->cash -= trade->get_units() * trade->get_average_price();

    //insert the trade into the portfolio
    this->portfolio.insert({trade->get_asset_id(), trade});
}

void Account::close_trade(const string& asset_id) {
    //find the trade in the portfolio and remove it
    auto trade = this->portfolio.at(asset_id);
    this->portfolio.erase(asset_id);

#ifdef ARGUS_RUNTIME_ASSERT
    assert(!trade->get_is_open());
#endif

    //update account's realized pl and cash
    this->realized_pl += trade->get_realized_pl();
}
