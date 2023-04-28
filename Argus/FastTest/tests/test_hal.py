import sys
import os
import unittest
sys.path.append(os.path.abspath('..'))

import Asset
from Hal import Hal

import FastTest
from FastTest import Portfolio, Exchange, Broker

import helpers

class SimpleStrategy:
    def __init__(self, hal : Hal) -> None:
        self.exchange = hal.get_exchange(helpers.test1_exchange_id)
        self.broker = hal.get_broker(helpers.test1_broker_id)
        
        self.portfolio1 = hal.new_portfolio("test_portfolio1",10000.0);
        self.portfolio2 = hal.new_portfolio("test_portfolio2",10000.0);  
        
    def on_open(self) -> None:
        return
    
    def on_close(self) -> None:
        position = self.portfolio1.get_position(helpers.test2_asset_id)
        
        if position is None:
            pass
            
        
    
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



if __name__ == '__main__':
    unittest.main()