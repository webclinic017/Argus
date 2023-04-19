import os
import sys
from typing import Type
import numpy as np
import pandas as pd
from ctypes import *

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.dirname(SCRIPT_DIR))

from FastTest import Asset
import FastTest


def asset_from_df(df: Type[pd.DataFrame], asset_id : str) -> Asset:
    values = df.values.flatten().astype(np.float64)
    epoch_index = df.index.values.astype(np.float64)

    values_p = values.ctypes.data_as(POINTER(c_double))
    epoch_index_p = epoch_index.ctypes.data_as(POINTER(c_longlong))

    asset = FastTest.new_asset(asset_id)
    asset.load_headers(df.columns.tolist())
    asset.load_data(values_p, epoch_index_p, df.shape[0], df.shape[1])

    return asset
