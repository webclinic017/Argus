import helpers
import Asset
import sys
import os
import unittest
import numpy as np

import FastTest

sys.path.append(os.path.abspath('..'))

class AssetTestMethods(unittest.TestCase):
    def test_mp_new_portfolio(self):
        hydra = FastTest.new_hydra(0)
        portfolio = hydra.new_portfolio("test_portfolio",100.0);
        assert(True)
        
    def test_sub_portfolio_create(self):
        hydra = FastTest.new_hydra(0)
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        portfolio3 = portfolio1.create_sub_portfolio("test_portfolio3",100.0);
        
        mp = hydra.get_master_portfolio();
        portfolio1_search = mp.find_portfolio("test_portfolio1");
                
        address_1 = portfolio1.get_mem_address()
        address_2 = portfolio1_search.get_mem_address()
        
        assert(address_1 == address_2)

        portfolio3_search_mp = mp.find_portfolio("test_portfolio3");
        portfolio3_search_1 = portfolio1.find_portfolio("test_portfolio3");
        
        assert(portfolio3.get_mem_address() == portfolio3_search_mp.get_mem_address() == portfolio3_search_1.get_mem_address())
            
    def test_portfolio_order_prop(self):
        hydra = helpers.build_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        portfolio3 = portfolio1.create_sub_portfolio("test_portfolio3",100.0);
        
        hydra.forward_pass()

        portfolio2.place_market_order(
            helpers.test2_asset_id,
            100.0,
            helpers.test1_exchange_id,
            helpers.test1_broker_id,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        p1 = portfolio2.get_position(helpers.test2_asset_id)
        p2 = mp.get_position(helpers.test2_asset_id)

        assert(p1 is not None)
        assert(p2 is not None)    
        assert(portfolio3.get_position(helpers.test2_asset_id) is None)
        assert(portfolio1.get_position(helpers.test2_asset_id) is None)
        
        assert(p1.get_units() == 100.0)
        assert(p1.get_average_price() == 101.0)
        assert(p2.get_units() == 100.0)
        assert(p2.get_average_price() == 101.0)
        
        trade1 = p1.get_trade(0)
        trade_mp = p2.get_trade(0)
        
        assert(trade1 is not None)
        assert(trade_mp is not None)    
        assert(trade1.get_mem_address() == trade_mp.get_mem_address())
        assert(trade1.get_units() == 100.0)
        assert(trade1.get_average_price() == 101.0)


    def test_portfolio_order_increase(self):
        hydra = helpers.build_simple_hydra(logging=1)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        
        hydra.forward_pass()
        
        portfolio2.place_market_order(
            helpers.test2_asset_id,
            100.0,
            helpers.test1_exchange_id,
            helpers.test1_broker_id,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )

        portfolio1.place_market_order(
            helpers.test2_asset_id,
            50.0,
            helpers.test1_exchange_id,
            helpers.test1_broker_id,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        p_mp = mp.get_position(helpers.test2_asset_id) 
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(p1.get_units() == 50.0)
        assert(p2.get_units() == 100.0)
        assert(p_mp.get_units() == 150.0)
        
        
        portfolio2.place_market_order(
            helpers.test2_asset_id,
            -100.0,
            helpers.test1_exchange_id,
            helpers.test1_broker_id,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        
        assert(True)

                
if __name__ == '__main__':
    unittest.main()
