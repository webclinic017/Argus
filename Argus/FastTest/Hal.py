import sys
import os
from datetime import datetime
from typing import Type, List
import cProfile

import pandas as pd
import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
from FastTest import Broker, Exchange, Asset, Portfolio, Hydra

class Hal:
    def __init__(self, logging : int, cash : float = 0.0) -> None:
        self.hydra = FastTest.Hydra(logging, cash)
        self.logging = logging        
        self.strategies = {}
        self.is_built = False
        
    def build(self):
        self.hydra.build()
        self.is_built = True
        
    def reset(self, clear_history = True, clear_strategies = False):
        if clear_strategies:
            self.strategies = {}
        self.hydra.reset(clear_history, clear_strategies)
    
    def reset_strategies(self):
        self.hydra.reset_strategies()
        
    def replay(self):
        self.hydra.replay()
        
    def goto_datetime(self, datetime : str = ""):
        to_epoch = pd.to_datetime(datetime).value
        self.hydra.goto_datetime(to_epoch)
        
    def get_hydra(self) -> FastTest.Hydra:
        return self.hydra
        
    def new_broker(self, broker_id : str, cash : float) -> FastTest.Broker:
        return self.hydra.new_broker(broker_id, cash)
    
    def new_exchange(self, exchange_id : str) -> FastTest.Exchange:
        return self.hydra.new_exchange(exchange_id)
    
    def get_broker(self, broker_id : str) -> FastTest.Broker:
        return self.hydra.get_broker(broker_id)
    
    def get_exchange(self, exchange_id : str) -> FastTest.Exchange:
        return self.hydra.get_exchange(exchange_id)
    
    def get_portfolio(self, portfolio_id : str) -> FastTest.Portfolio:
        return self.hydra.get_portfolio(portfolio_id)
    
    def get_candles(self) -> int:
        return self.hydra.get_candles()
        
    def new_portfolio(self, portfolio_id : str, cash : float, parent_portfolio_id : str = "master") -> Portfolio:
        parent_portfolio = self.hydra.get_portfolio(parent_portfolio_id)
        return parent_portfolio.create_sub_portfolio(portfolio_id, cash)
        
    def register_asset(self, asset : Asset, exchange_id : str) -> None:
        exchange = self.hydra.get_exchange(exchange_id)
        exchange.register_asset(asset)
        
    def register_strategy(self, py_strategy, strategy_id : str) -> None:
        for attr in ["on_open","on_close","build"]:
            if not hasattr(py_strategy, attr):
                raise RuntimeError(f"strategy must implement {attr}()")
            
        if strategy_id in self.strategies.keys():
            raise RuntimeError("strategy id already exists")
            
        strategy = self.hydra.new_strategy()
        strategy.on_open = py_strategy.on_open
        strategy.on_close = py_strategy.on_close    
                
    def profile(self):
        pr = cProfile.Profile()
        pr.enable()
        self.run()
        pr.disable()
        pr.print_stats(sort='cumulative')
        
    def get_hydra_time(self):
        return datetime.fromtimestamp(self.hydra.get_hydra_time())

    def run(self, to : str = "", steps : int = 0):
        #note, if two conditions passed the first to tragger will casue hault
        if not self.is_built:
            raise RuntimeError("Hal has not been built")
        if to == "":
            to_epoch = 0
        else:
            to_epoch = pd.to_datetime(to).value
            
        self.hydra.run(to_epoch, steps)
        
    def register_asset_from_df(self,
                            df: Type[pd.DataFrame],
                            asset_id : str,
                            exchange_id : str,
                            broker_id : str,
                            warmup : int):
        """register an load in a new asset from a pandas dataframe

        Args:
            df (Type[pd.DataFrame]): panda dataframe to laod in
            asset_id (str): unique id of the new asset
            exchange_id (str): unique id of the exchange to place the asset on
            broker_id (str): unique id of the broker to place the asset on
        """
        asset = asset_from_df(df, asset_id, exchange_id, broker_id, warmup)
        self.register_asset(asset, exchange_id)
        
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
    
           
def asset_from_df(df: Type[pd.DataFrame], 
                asset_id: str,
                exchange_id : str,
                broker_id : str,
                warmup = 0) -> Asset:
    """generate a new asset object from a pandas dataframe. Pandas index must have a datetime
    index. An easy conversion from datetime str as: df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace=True)

    Args:
        df (Type[pd.DataFrame]): new pandas dataframe to create asset with
        asset_id (str): unique id of the new asset
        exchange_id (str): unique id of the exchange to place the asset on
        broker_id (str): unique id of the broker to place the asset on
    Returns:
        Asset: a new Asset object
    """
    # extract underlying numpy arrays
    values = df.values.astype(np.float64)
    epoch_index = df.index.values.astype(np.int64)

    # load the asset
    asset = FastTest.new_asset(asset_id, exchange_id, broker_id, warmup)
    asset.load_headers(df.columns.tolist())
    asset.load_data(values, epoch_index, df.shape[0], df.shape[1], False)

    return asset