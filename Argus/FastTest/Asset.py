import FastTest
import os
import sys
from typing import Type, List
import numpy as np
import pandas as pd
from ctypes import *

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

def asset_from_df(df: Type[pd.DataFrame], asset_id: str, exchange_id : str, broker_id : str):
    # extract underlying numpy arrays
    values = df.values.astype(np.float64)
    epoch_index = df.index.values.astype(np.int64)

    # load the asset
    asset = FastTest.new_asset(asset_id, exchange_id, broker_id)
    asset.load_headers(df.columns.tolist())
    asset.load_data(values, epoch_index, df.shape[0], df.shape[1], False)

    return asset

def asset_from_view(
    data : np.array,
    datetime_index : np.array,
    asset_id : str, exchange_id : str, broker_id : str,
    columns: List,
        ):
    
    rows_ = int(data.shape[0] / len(columns))
    cols_ = len(columns)
    
    asset = FastTest.new_asset(asset_id, exchange_id, broker_id)
    asset.load_headers(columns)
    asset.load_data(data, datetime_index, rows_, cols_, True)
    
    return asset
