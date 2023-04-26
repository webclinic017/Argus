import numpy as np
import pandas as pd
import sys
import os
sys.path.append(os.path.abspath('..'))

import Asset
import FastTest

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

def build_simple_hydra(logging = 0):
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
    hydra.build()
    return hydra
