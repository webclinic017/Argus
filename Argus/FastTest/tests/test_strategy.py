import sys
import os
import time
import unittest
import cProfile
import numpy as np
sys.path.append(os.path.abspath('..'))

import Asset
import FastTest
from Hal import Hal
from FastTest import Portfolio, Exchange, Broker, Hydra, Strategy

import helpers

class SimpleStrategy:
    def __init__(self, hydra : Hydra) -> None:
        self.strategy = hydra.new_strategy()
        self.strategy.on_open = self.on_open
        self.strategy.on_close = self.on_close
        self.x = 0
                
    def on_open(self) -> None:
        self.x += 1
    
    def on_close(self) -> None:
        return

    def build(self) -> None:
        return

class StrategyTestMethods(unittest.TestCase):
    def test_hal_run(self):
        hal = helpers.create_simple_hal(logging=0)
      
        stategy = SimpleStrategy(hal.get_hydra())
        
        hydra = hal.get_hydra()
        hydra.build()
        hydra.run()
        
        assert(stategy.x == 6)
      
        assert(True)
        
        
if __name__ == '__main__':
    unittest.main()