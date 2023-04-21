import gc
import sys
import os
import unittest

import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset
import helpers

class ExchangeTestMethods(unittest.TestCase):
    def test_exchange_datetime_index(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )

        hydra = FastTest.new_hydra()
        exchange = hydra.new_exchange("exchange1")
        exchange.register_asset(asset1)
        exchange.build()
        exchange_index = exchange.get_datetime_index_view()
        assert(np.array_equal(exchange_index, asset1.get_datetime_index_view()))

    def test_exchange_multi_datetime_index(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            helpers.test1_asset_id
        )
        asset2 = helpers.load_asset(
            helpers.test2_file_path,
            helpers.test2_asset_id,
        )

        hydra = FastTest.new_hydra()
        exchange = hydra.new_exchange("exchange1")
        exchange.register_asset(asset1)
        exchange.register_asset(asset2)
        exchange.build()
        exchange_index = exchange.get_datetime_index_view()

        assert(np.array_equal(exchange_index, asset2.get_datetime_index_view()))

if __name__ == '__main__':
    unittest.main()
