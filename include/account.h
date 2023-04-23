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

class Account;
typedef tsl::robin_map<string,Account> Accounts;

class Account{
private:
    ///unique id of the account
    string account_id;

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
    /// account constructor
    /// \param account_id unique id of the account
    /// \param starting_cash amount of starting cash in the account
    Account(string account_id, double starting_cash);

    /// handle a new trade created for the account
    /// \param trade smart pointer to a a new trade
    void new_trade(const shared_ptr<Trade>& trade);

    void close_trade(const string & asset_id);

};

#endif //ARGUS_ACCOUNT_H
