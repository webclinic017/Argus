import sys
import os
import time
import numpy as np
import unittest
sys.path.append(os.path.abspath('..'))

import Asset
import FastTest
from Hal import Hal
from FastTest import Portfolio, Exchange, Broker

import helpers

class SimpleStrategy:
    def __init__(self, hal : Hal) -> None:
        self.exchange = hal.get_exchange(helpers.test1_exchange_id)
        self.broker = hal.get_broker(helpers.test1_broker_id)
        self.portfolio1 = hal.new_portfolio("test_portfolio1",100000.0);
        
    def on_open(self) -> None:
        return
    
    def on_close(self) -> None:
        position = self.portfolio1.get_position(helpers.test2_asset_id)
        close_price = self.exchange.get_asset_feature(helpers.test2_asset_id, "CLOSE")
        if position is None:
            if close_price <= 97.0:
                self.portfolio1.place_market_order(
                    helpers.test2_asset_id,
                    100.0,
                    helpers.test1_exchange_id,
                    helpers.test1_broker_id,
                    "dummy",
                    FastTest.OrderExecutionType.EAGER,
                    -1
                )
        elif close_price >= 101.5:
            self.portfolio1.place_market_order(
                    helpers.test2_asset_id,
                    -1 * position.get_units(),
                    helpers.test1_exchange_id,
                    helpers.test1_broker_id,
                    "dummy",
                    FastTest.OrderExecutionType.EAGER,
                    -1
                )

    def build(self) -> None:
        return
        
class HalTestMethods(unittest.TestCase):
    def test_hal_run(self):
        hal = helpers.create_simple_hal(logging=0)
        hal.build()
        hal.run()
        assert(True)
        
    def test_hal_register_strategy(self):
        hal = helpers.create_simple_hal(logging=0)

        strategy = SimpleStrategy(hal)
        hal.register_strategy(strategy)
        
        hal.build()
        hal.run()
                
        cash_actual = np.array([100000, 100000,  90300, 100450, 100450,  90850.0,])
        nlv_actual = np.array([100000, 100000, 100000, 100450, 100450, 100450.0,])
        
        for portfolio in ["master", "test_portfolio1"]:
            portfolio_history = hal.get_portfolio(portfolio).get_portfolio_history()
            cash_history = portfolio_history.get_cash_history()
            nlv_history = portfolio_history.get_nlv_history()
            
            assert(np.array_equal(cash_actual, cash_history))
            assert(np.array_equal(nlv_actual, nlv_history))
  
    def test_hal_big(self):
        hal = helpers.create_big_hal(logging=0)
        
        hal.build()
        st = time.time()
        hal.run()
        et = time.time()
        
        print(et - st)
        
        assert(True)

if __name__ == '__main__':
    unittest.main()