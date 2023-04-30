from typing import Type
import multiprocessing
import cProfile
import pickle

import pandas as pd
import numpy as np

import FastTest
from FastTest import Broker, Exchange, Asset, Portfolio, Hydra

class Hal:
    def __init__(self, logging : int) -> None:
        self.hydra = FastTest.Hydra(logging)
        self.logging = logging
        self.strategies = np.array([], dtype="O")
        self.is_built = False
        
    def build(self):
        self.hydra.build()
        self.is_built = True
        
    def get_hydra(self) -> Hydra:
        return self.hydra
        
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
    
    def get_candles(self) -> int:
        return self.hydra.get_candles()
    
    def get_order_history(self):
        orders = self.hydra.get_order_history()
        orders_list = []
        for order in orders:
            orders_list.append({
                "fill_time" : order.get_fill_time(),
                "asset_id":  order.get_asset_id(),
                "portfolio_id" : order.get_portfolio_id(),
                "units" : order.get_units(),
                "strategy_id" : order.get_strategy_id(),
                "order_type": order.get_order_type(), 
                "order_state": order.get_order_state(),
                "average_price": order.get_average_price(),
                "order_id" : order.get_order_id(),
                "trade_id" : order.get_trade_id(),
                "exchange_id" : order.get_exchange_id(),
                "broker_id" : order.get_broker_id()         
            })
        df = pd.DataFrame.from_records(orders_list)
        df["fill_time"] = pd.to_datetime(df["fill_time"])
        return df

    def get_trade_history(self):
        return self.hydra.get_trade_history()
    
    def get_position_history(self):
        return self.hydra.get_position_history()
    
    def new_portfolio(self, portfolio_id : str, cash : float, parent_portfolio_id : str = "master") -> Portfolio:
        parent_portfolio = self.hydra.get_portfolio(parent_portfolio_id)
        return parent_portfolio.create_sub_portfolio(portfolio_id, cash)
        
    def register_asset(self, asset : Asset, exchange_id : str) -> None:
        exchange = self.hydra.get_exchange(exchange_id)
        exchange.register_asset(asset)
        
    def register_strategy(self, py_strategy) -> None:
        for attr in ["on_open","on_close","build"]:
            if not hasattr(py_strategy, attr):
                raise RuntimeError(f"strategy must implement {attr}()")
        
        strategy = self.hydra.new_strategy()
        strategy.on_open = py_strategy.on_open
        strategy.on_close = py_strategy.on_close    
                
    def profile(self):
        pr = cProfile.Profile()
        pr.enable()
        self.run()
        pr.disable()
        pr.print_stats(sort='cumulative')

        
    def run(self):
        if not self.is_built:
            raise RuntimeError("Hal has not been built")
        
        return self.hydra.run()
           