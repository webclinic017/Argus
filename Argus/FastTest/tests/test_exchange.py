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
            "asset1",
            helpers.test1_exchange_id,
            helpers.test1_broker_id
        )

        hydra = FastTest.Hydra(0, 0.0)
        exchange = hydra.new_exchange("exchange1")
        exchange.register_asset(asset1)
        exchange.build()
        exchange_index = exchange.get_datetime_index_view()
        assert(np.array_equal(exchange_index, asset1.get_datetime_index_view()))

    def test_exchange_multi_datetime_index(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            helpers.test1_asset_id,
            helpers.test1_exchange_id,
            helpers.test1_broker_id
        )
        asset2 = helpers.load_asset(
            helpers.test2_file_path,
            helpers.test2_asset_id,
            helpers.test1_exchange_id,
            helpers.test1_broker_id
        )

        hydra = FastTest.Hydra(0, 0.0)
        exchange = hydra.new_exchange("exchange1")
        exchange.register_asset(asset1)
        exchange.register_asset(asset2)
        exchange.build()
        exchange_index = exchange.get_datetime_index_view()

        assert(np.array_equal(exchange_index, asset2.get_datetime_index_view()))
    """
    def test_exchange_get_asset_feature(self):
        hydra = helpers.create_simple_hydra(logging=0)
        
        hydra.build()
        hydra.forward_pass()
        
        exchange = hydra.get_exchange(helpers.test1_exchange_id)
        
        assert(exchange.get_asset_feature(helpers.test2_asset_id, "CLOSE",0) == 101.5)
        assert(exchange.get_asset_feature(helpers.test2_asset_id, "CLOSE") == 101.5)
        
        hydra.backward_pass()
        hydra.forward_pass()
        
        assert(exchange.get_asset_feature(helpers.test2_asset_id, "CLOSE") == 99)
        assert(exchange.get_asset_feature(helpers.test2_asset_id, "OPEN") == 100)
        assert(exchange.get_asset_feature(helpers.test2_asset_id, "OPEN", -1) == 101)
        
    def test_exchange_get_exchange_feature(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        exchange = hydra.get_exchange(helpers.test1_exchange_id)
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        
        hydra.build()
        hydra.forward_pass()
        hydra.on_open()
        
        exchange_features = {}
        exchange.get_exchange_feature(exchange_features, "CLOSE")
        assert(exchange_features[helpers.test2_asset_id] == 101.5)
        assert(exchange_features[helpers.test1_asset_id] is None)
        
        hydra.backward_pass()
        hydra.forward_pass()
        
        exchange_features = {}
        exchange.get_exchange_feature(exchange_features, "CLOSE")
        assert(exchange_features[helpers.test2_asset_id] == 99.0)
        assert(exchange_features[helpers.test1_asset_id] == 101.0)
    """
        

                
if __name__ == '__main__':
    unittest.main()
