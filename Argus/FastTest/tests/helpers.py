import sys
import os
import pandas as pd
import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset

test1_file_path = "./data/test1.csv"
test1_asset_id = "asset_id1"

def load_asset(file_path, asset_id):
    df = pd.read_csv(file_path)
    df.set_index("DATE", inplace = True)
    df.set_index(pd.to_datetime(df.index).astype(np.int64), inplace = True)

    asset = Asset.asset_from_df(df, asset_id)

    return asset
