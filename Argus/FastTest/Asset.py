import FastTest
import os
import sys
from typing import Type
import numpy as np
import pandas as pd
from ctypes import *

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))


def asset_from_df(df: Type[pd.DataFrame], asset_id: str) -> FastTest.Asset:
    # extract underlying numpy arrays
    values = df.values.astype(np.float64)
    epoch_index = df.index.values.astype(np.int64)

    # load the asset
    asset = FastTest.new_asset(asset_id)
    asset.load_headers(df.columns.tolist())
    asset.load_data(values, epoch_index, df.shape[0], df.shape[1])

    return asset
