from typing import Type

import numpy as np

import FastTest
from FastTest import Broker, Exchange, Asset, Portfolio

class Hal:
    def __init__(self, logging : int) -> None:
        self.hydra = FastTest.Hydra(logging)
        self.logging = logging
        self.strategies = np.array([], dtype="O")
        self.is_built = False
        
    def build(self):
        self.hydra.build()
        self.is_built = True
        
    def new_broker(self, broker_id : str, cash : float) -> Broker:
        return self.hydra.new_broker(broker_id, cash)
    
    def new_exchange(self, exchange_id : str) -> Exchange:
        return self.hydra.new_exchange(exchange_id)
    
    def get_broker(self, broker_id : str) -> Broker:
        return self.hydra.get_broker(broker_id)
    
    def get_exchange(self, exchange_id : str) -> Exchange:
        return self.hydra.get_exchange(exchange_id)
    
    def get_portfolio(self, portfolio_id : str) -> Portfolio:
        return self.hydra.get_portfolio(portfolio_id)
    
    def new_portfolio(self, portfolio_id : str, cash : float, parent_portfolio_id : str = "master") -> Portfolio:
        parent_portfolio = self.hydra.get_portfolio(parent_portfolio_id)
        return parent_portfolio.create_sub_portfolio(portfolio_id, cash)
        
    def register_asset(self, asset : Asset, exchange_id : str) -> None:
        exchange = self.hydra.get_exchange(exchange_id)
        exchange.register_asset(asset)
        
    def register_strategy(self, strategy) -> None:
        for attr in ["on_open","on_close","build"]:
            if not hasattr(strategy, attr):
                raise RuntimeError(f"strategy must implement {attr}()")
            
        self.strategies = np.append(self.strategies,(strategy))
        
    def run(self):
        if not self.is_built:
            raise RuntimeError("Hal has not been built")
            
        while True:
            #allow brokers to process orders that have been filled or orders that were placed by 
            #strategies with lazy execution. 
            self.hydra.forward_pass()
            
            for strategy in self.strategies:
                strategy.on_open()
            
            self.hydra.on_open()
            
            for strategy in self.strategies:
                strategy.on_close()
            
            if not self.hydra.backward_pass():
                return True    