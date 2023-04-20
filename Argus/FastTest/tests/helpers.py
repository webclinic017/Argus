import sys
import os
import pandas as pd
import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset

test1_file_path = "./data/test1.csv"
test_spy_file_path = "./data/SPY_DAILY.csv"

test1_asset_id = "asset_id1"
spy_asset_id = "asset_id1"

def load_df(file_path, asset_id):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace = True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace = True)
    return df


def load_asset(file_path, asset_id, is_view = False):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace = True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace = True)

    asset = Asset.asset_from_df(df, asset_id, is_view)

    return asset
