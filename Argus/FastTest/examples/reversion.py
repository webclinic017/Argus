import sys
import os
import time

import matplotlib.pyplot as plt
import pandas as pd

sys.path.append(os.path.abspath('..'))

import helpers
import FastTest
from FastTest import PortfolioTracerType, ExchangeQueryType
from Hal import Hal

class TrendStrategy:
    def __init__(self, hal : Hal) -> None:        
        self.exchange = hal.get_exchange(exchange_id)
        self.broker = hal.get_broker(broker_id)
        self.portfolio = hal.get_portfolio("master");
                
    def build(self) -> None:
        return
                
    def on_open(self) -> None:
        return
    
    def on_close(self) -> None:
        exchange_features =  exchange.get_exchange_feature("50MA_SPREAD", query_type = ExchangeQueryType.NSMALLEST, N = 10)
        allocations = {asset_id : .1 for asset_id in exchange_features.keys()}
        self.portfolio.order_target_allocations(
            allocations,
            "trend_strategy",
            .00,
        )
        
logging = 0
cash = 100000
dir_path = "/Users/nathantormaschy/CLionProjects/Argus/Argus/FastTest/tests/SP500_D"
hal = Hal(logging, cash)

exchange_id = "exchange1"
broker_id = "broker1"

broker = hal.new_broker(broker_id,100000.0)
exchange = hal.new_exchange(exchange_id)

file_list = [os.path.join(dir_path, f) for f in os.listdir(dir_path) if os.path.isfile(os.path.join(dir_path, f))]
candles = 0
for _file in file_list:
    _file_base = os.path.basename(_file)
    asset_id = os.path.splitext(_file_base)[0]
    
    df = pd.read_feather(_file)
    df["Date"] = df["Date"] * 1e9
    df.set_index("Date", inplace=True)
    df.dropna(inplace = True)
    df["50MA_SPREAD"] = (df["Close"] - df["Close"].rolling(50).mean())
    df = df.iloc[200:]
    
    hal.register_asset_from_df(df, asset_id, exchange_id, broker_id, warmup = 0) 
    candles += len(df)
        
strategy = TrendStrategy(hal)
hal.register_strategy(strategy, "trend_strategy") 

hal.build()
st = time.time()
hal.run()
et = time.time()

mp = hal.get_portfolio("master")
nlv = mp.get_tracer(PortfolioTracerType.VALUE).get_nlv_history()
    
print(f"FastTest completed in {et-st:.6f} seconds")
print(f"FastTest Final Portfolio Value: {nlv[-1]:,.4f}")
print(f"HAL: candles per seoncd: {(candles / (et-st)):,.3f}")   


plt.plot(nlv)
plt.show()