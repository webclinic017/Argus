import numpy as np
import pandas as pd
import sys
import os
sys.path.append(os.path.abspath('..'))

import FastTest
from Hal import Hal, asset_from_df

def load_df(file_path, asset_id):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace=True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace=True)
    return df

def load_asset(file_path, asset_id, exchange_id, broker_id, is_view=False):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace=True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace=True)

    asset = asset_from_df(df, asset_id, exchange_id, broker_id)

    return asset

def create_index_hal(dir_path : str, logging: int = 0, cash: float = 0) -> Hal:
    hal = Hal(logging, cash)
    broker = hal.new_broker("broker1",0.0)
    exchange = hal.new_exchange("exchange1")
    

    file_list = [os.path.join(dir_path, f) for f in os.listdir(dir_path) if os.path.isfile(os.path.join(dir_path, f))]
    for _file in file_list:
        _file_base = os.path.basename(_file)
        asset_id = os.path.splitext(_file_base)[0]
        
        df = pd.read_feather(_file)
        df["Date"] = df["Date"] * 1e9
        df.set_index("Date", inplace=True)
        df.dropna(inplace = True)
        
        hal.register_asset_from_df(df, asset_id, "exchange1", "broker1", warmup = 0) 
    return hal