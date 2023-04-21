//
// Created by Nathan Tormaschy on 4/21/23.
//

#ifndef ARGUS_ACCOUNT_H
#define ARGUS_ACCOUNT_H
#include <string>
#include <memory>
#include <tsl/robin_map.h>

#include "trade.h"

using namespace std;

class Account{
private:
    ///amount of cash initially held by the account
    double starting_cash;

    ///amount of cash held by the account
    double cash;

    ///net liquidation value of the account
    double nlv;

    ///unrealize pl of the account
    double unrealized_pl;

    ///realized pl of the account
    double realized_pl;

    ///trades help by the account
    tsl::robin_map<string,shared_ptr<Trade>> portfolio;

public:
    Account(string account_id, double starting_cash);

};

#endif //ARGUS_ACCOUNT_H
