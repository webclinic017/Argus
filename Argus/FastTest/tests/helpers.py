import numpy as np
import pandas as pd
import sys
import os
sys.path.append(os.path.abspath('..'))

import Asset
import FastTest
from Hal import Hal

test1_file_path = "./data/test1.csv"
test2_file_path = "./data/test2.csv"

test_spy_file_path = "./data/SPY_DAILY.csv"

test1_exchange_id = "exchange_id1"
test1_broker_id = "broker_id1"

test1_asset_id = "asset_id1"
test2_asset_id = "asset_id2"


spy_asset_id = "asset_id1"


def load_df(file_path, asset_id):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace=True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace=True)
    return df


def load_asset(file_path, asset_id, is_view=False):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace=True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace=True)

    asset = Asset.asset_from_df(df, asset_id)

    return asset

def create_simple_hydra(logging = 0) -> FastTest.Hydra:
    asset1 = load_asset(
        test1_file_path,
        test1_asset_id
    )
    asset2 = load_asset(
        test2_file_path,
        test2_asset_id,
    )

    hydra = FastTest.Hydra(logging)
    broker = hydra.new_broker(test1_broker_id,100000.0)
    exchange = hydra.new_exchange(test1_exchange_id)
    exchange.register_asset(asset1)
    exchange.register_asset(asset2)
    return hydra

def create_simple_hal(logging: int = 0) -> Hal:
    asset1 = load_asset(
        test1_file_path,
        test1_asset_id
    )
    asset2 = load_asset(
        test2_file_path,
        test2_asset_id,
    )
    
    hal = Hal(logging)
    broker = hal.new_broker(test1_broker_id,100000.0)
    exchange = hal.new_exchange(test1_exchange_id)
    exchange.register_asset(asset1)
    exchange.register_asset(asset2)
    
    return hal 

def create_big_hal(logging: int = 0) -> Hal:
    hal = Hal(logging)
    broker = hal.new_broker(test1_broker_id,100000.0)
    exchange = hal.new_exchange(test1_exchange_id)
    
    dir_path = os.path.abspath(os.path.join(os.path.dirname(__file__)))
    data_path = os.path.join(dir_path, "SP500_D")
    file_list = [os.path.join(data_path, f) for f in os.listdir(data_path) if os.path.isfile(os.path.join(data_path, f))]

    for _file in file_list[0:1]:
        _file_base = os.path.basename(_file)
        asset_id = os.path.splitext(_file_base)[0]
        
        df = pd.read_feather(_file)
        df["Date"] = df["Date"] * 1e9
        df.set_index("Date", inplace=True)
        
        df["FAST_ABOVE_SLOW"] = df["Close"].rolling(50).mean() >  df["Close"].rolling(200).mean()
        df.dropna(inplace = True)
        
        asset = Asset.asset_from_df(df, asset_id)
        exchange.register_asset(asset)    
    return hal
