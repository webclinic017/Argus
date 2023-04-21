//
// Created by Nathan Tormaschy on 4/21/23.
//
#include <string>
#include "account.h"

Account::Account(string account_id, double starting_cash) {
    this->starting_cash = starting_cash;
    this->cash = starting_cash;
    this->nlv = starting_cash;
    this->unrealized_pl = 0.0;
    this->realized_pl = 0.0;
    this->portfolio = {};
}