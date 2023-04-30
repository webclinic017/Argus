import helpers
import Asset
import sys
import os
import unittest
import numpy as np

import FastTest

sys.path.append(os.path.abspath('..'))

def test_gc_helper():
    df = helpers.load_df(helpers.test1_file_path, helpers.test1_asset_id)
    asset = Asset.asset_from_df(df, helpers.test1_asset_id)
    return asset

class AssetTestMethods(unittest.TestCase):
    def test_asset_load(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1",
            helpers.test1_exchange_id,
            helpers.test1_broker_id
        )
        assert (True)

    def test_asset_load_view(self):
        df = helpers.load_df(helpers.test1_file_path, helpers.test1_asset_id)
        
        values = df.values.flatten().astype(np.float64)
        epoch_index = df.index.values.astype(np.int64)
        columns = df.columns
        
        asset = Asset.asset_from_view(values, epoch_index, 
                                    helpers.test1_asset_id, 
                                    helpers.test1_exchange_id,
                                    helpers.test1_broker_id,
                                    columns)

        assert (asset.get("CLOSE", 0) == 101)
        assert (asset.get("OPEN", 3) == 105)
        
        values[1] = 123
        
        assert (asset.get("CLOSE", 0) == 123)

    def test_asset_get(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            helpers.test1_exchange_id,
            helpers.test1_broker_id,
            "asset1"
        )
        assert (asset1.get("CLOSE", 0) == 101)
        assert (asset1.get("OPEN", 3) == 105)

    def test_asset_memory_address(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1",
            helpers.test1_exchange_id,
            helpers.test1_broker_id
        )

        hydra = FastTest.new_hydra(0)
        exchange = hydra.new_exchange("exchange1")

        # register the existing asset to the exchange, the asset in the exchange's market
        # should have the same meory address as asset1 created above
        exchange.register_asset(asset1)
        asset2 = exchange.get_asset("asset1")

        address_1 = asset1.get_mem_address()
        address_2 = asset2.get_mem_address()

        assert (address_1 == address_2)


if __name__ == '__main__':
    unittest.main()
