import gc
import sys
import os
import unittest
import ctypes
import time
import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset
import helpers

def test_gc_helper():
    df = helpers.load_df(helpers.test1_file_path, helpers.test1_asset_id)
    asset = Asset.asset_from_df(df, helpers.test1_asset_id, is_view = True)
    return asset


class AssetTestMethods(unittest.TestCase):
    def test_asset_load(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )
        assert(True);

    def test_asset_load_view(self):
        df = helpers.load_df(helpers.test1_file_path, helpers.test1_asset_id)
        asset = Asset.asset_from_df(df, helpers.test1_asset_id, is_view = True)

        assert(asset.get("CLOSE",0) == 101)
        assert(asset.get("OPEN",3) == 105)

    def test_asset_get(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )
        assert(asset1.get("CLOSE",0) == 101)
        assert(asset1.get("OPEN",3) == 105)

    def test_asset_memory_address(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )

        hydra = FastTest.new_hydra()
        exchange = hydra.new_exchange("exchange1")

        #register the existing asset to the exchange, the asset in the exchange's market
        #should have the same meory address as asset1 created above
        exchange.register_asset(asset1)
        asset2 = exchange.get_asset("asset1")

        address_1 = FastTest.mem_address(asset1)
        address_2 = FastTest.mem_address(asset1)

        assert(address_1 == address_2)

if __name__ == '__main__':
    unittest.main()
