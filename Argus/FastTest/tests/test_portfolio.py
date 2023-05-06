import sys
import os
import time
import unittest
import numpy as np

sys.path.append(os.path.abspath('..'))

import FastTest
from FastTest import OrderExecutionType, OrderTargetType, PortfolioTracerType

import helpers

class PortfolioTestMethods(unittest.TestCase):
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
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        portfolio3 = portfolio1.create_sub_portfolio("test_portfolio3",100.0);
        
        hydra.build()
        hydra.forward_pass()

        portfolio2.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        p_mp = mp.get_position(helpers.test2_asset_id)

        assert(p2 is not None)
        assert(p_mp is not None)    
        assert(portfolio3.get_position(helpers.test2_asset_id) is None)
        assert(portfolio1.get_position(helpers.test2_asset_id) is None)
        
        assert(p2.get_units() == 100.0)
        assert(p2.get_average_price() == 101.0)
        assert(p1 is None)
        
        #static trade counter
        #trade1 = p2.get_trade(0)
        #trade_mp = p_mp.get_trade(0)
        
        ##assert(trade1 is not None)
        #assert(trade_mp is not None)    
        #assert(trade1.get_mem_address() == trade_mp.get_mem_address())
        #assert(trade1.get_units() == 100.0)
        #assert(trade1.get_average_price() == 101.0)

    def test_portfolio_order_increase(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100.0);  
        
        hydra.build()
        hydra.forward_pass()
        
        portfolio2.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        portfolio1.place_market_order(
            helpers.test2_asset_id,
            50.0,
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
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        assert(portfolio2.get_position(helpers.test2_asset_id) is None)
        assert(not p2.is_open)
        assert(p_mp.get_units() == 50)
        assert(p1.get_units() == 50)
        
    def test_portfolio_eval(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",10000.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",10000.0);  
        
        hydra.build()
        hydra.forward_pass()
        
        portfolio2.place_market_order(
            helpers.test2_asset_id,
            -100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        portfolio1.place_market_order( 
            helpers.test2_asset_id,
            50.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        hydra.on_open()
        hydra.backward_pass()
        
        p0 = mp.get_position(helpers.test2_asset_id)
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        
        assert(p0.get_units() == -50.0)
        assert(p1.get_units() == 50.0)
        assert(p2.get_units() == -100.0)
        
        assert(p0.get_unrealized_pl() == (-50.0 * .5))
            
        assert(mp.get_unrealized_pl() == (-50.0 * .5))
        assert(portfolio1.get_unrealized_pl() == (50.0 * .5))
        assert(portfolio2.get_unrealized_pl() == (-100.0 * .5))
        
        assert(portfolio1.get_cash() == (10000 - (50 * 101)))
        assert(portfolio2.get_cash() == (10000 - (-100 * 101)))
        assert(mp.get_cash() == portfolio1.get_cash() + portfolio2.get_cash())
        
        assert(portfolio1.get_nlv() == (10000 + (50 * .5)))
        assert(portfolio2.get_nlv() == (10000 + (-100 * .5)))
        assert(mp.get_nlv() == portfolio1.get_nlv() + portfolio2.get_nlv())
        
            
    def test_portfolio_order_traget_size(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",10000.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",10000.0);  
        
        hydra.build()
        hydra.forward_pass()
        
        portfolio2.order_target_size(helpers.test2_asset_id, .01, "dummy", 
                .01,
                FastTest.OrderTargetType.PCT,
                FastTest.OrderExecutionType.EAGER,
                -1)
        p2 = portfolio2.get_position(helpers.test2_asset_id)     
        assert(p2 is not None)
        assert(p2.units == (10000 * .01)/ 101)
        
        portfolio1.order_target_size(helpers.test2_asset_id, 10, "dummy", 
                .01,
                FastTest.OrderTargetType.DOLLARS,
                FastTest.OrderExecutionType.LAZY,
                -1)
        
        p1 = portfolio1.get_position(helpers.test2_asset_id)        
        assert(p1 is None) # lazy exeution
        hydra.on_open()
        p1 = portfolio1.get_position(helpers.test2_asset_id)     
        assert(p1 is not None)
        assert(p1.units == 10 / 101)
            
    def test_portfolio_close_position(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100.0);
        
        hydra.build()
        hydra.forward_pass()

        portfolio1.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        assert(p1 is not None)
        portfolio1.close_position(helpers.test2_asset_id)
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        assert(p1 is  None)
        
        hydra.on_open()
        hydra.backward_pass()
        
        hydra.forward_pass()
        
        portfolio1.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        portfolio1.place_market_order(
            helpers.test1_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio1.get_position(helpers.test1_asset_id)
        assert(p1 is not None)
        assert(p2 is not None)
        
        portfolio1.close_position()
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio1.get_position(helpers.test1_asset_id)
        assert(p1 is None)
        assert(p2 is None)
        
    def test_parent_portfolio_close(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100000.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100000.0);
        
        hydra.build()
        hydra.forward_pass()
        
        portfolio1.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        portfolio2.place_market_order(
            helpers.test2_asset_id,
            -50.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        mp.place_market_order(
            helpers.test2_asset_id,
            -50.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        mp = mp.get_position(helpers.test2_asset_id)
        p1 = portfolio1.get_position(helpers.test2_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(mp is None)
        assert(p1 is None)
        assert(p2 is None)
        
    def test_portfolio_target_allocations_order(self):
        hydra = helpers.create_simple_hydra(logging=0)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100000.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100000.0);
        
        hydra.build()
        hydra.forward_pass()
        hydra.on_open()
        hydra.backward_pass()
        
        hydra.forward_pass()
        
        allocations = {helpers.test1_asset_id : 100, helpers.test2_asset_id : -100}
        portfolio1.order_target_allocations(
            allocations,
            "dummy",
            .01,
            order_target_type = OrderTargetType.UNITS
        )
        
        allocations = {helpers.test1_asset_id : -50, helpers.test2_asset_id : 50}
        portfolio2.order_target_allocations(
            allocations,
            "dummy",
            .01,
            order_target_type = OrderTargetType.UNITS
        )
        
        hydra.on_open()
        hydra.backward_pass()
        
        pos = portfolio1.get_position(helpers.test1_asset_id)
        assert(pos.get_nlv() == 100 * 101)
        pos = portfolio1.get_position(helpers.test2_asset_id)
        assert(pos.get_nlv() == -100 * 99)
        
        pos = portfolio2.get_position(helpers.test1_asset_id)
        assert(pos.get_nlv() == -50 * 101)
        pos = portfolio2.get_position(helpers.test2_asset_id)
        assert(pos.get_nlv() == 50 * 99)
        
        pos = mp.get_position(helpers.test1_asset_id)
        assert(pos.get_nlv() == 50 * 101)
        pos = mp.get_position(helpers.test2_asset_id)
        assert(pos.get_nlv() == -50 * 99)
                
        portfolio1_nlv1 =  100000 + (100 * (101-100)) + (-100 * (99 - 100))
        portfolio2_nlv1 =  100000 + (-50 * (101-100)) + (50 * (99 - 100))
        mp_nlv1 = 200000 + (50 * (101-100)) + (-50 * (99 - 100))
        assert(portfolio1.get_nlv() == portfolio1_nlv1)
        assert(portfolio2.get_nlv() == portfolio2_nlv1)
        assert(mp.get_nlv() == mp_nlv1)
        
        hydra.forward_pass()
        allocations = {helpers.test1_asset_id : -100, helpers.test2_asset_id : 100}
        portfolio1.order_target_allocations(
            allocations,
            "dummy",
            .01,
            order_target_type = OrderTargetType.UNITS
        )
        
        allocations = {helpers.test1_asset_id : 50, helpers.test2_asset_id : -50}
        portfolio2.order_target_allocations(
            allocations,
            "dummy",
            .01,
            order_target_type = OrderTargetType.UNITS
        )
        print()
        hydra.on_open()
        hydra.backward_pass()
        
        p1 = portfolio2.get_position(helpers.test1_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(p1.units == 50)
        assert(p2.units == -50)
        
        p1 = portfolio1.get_position(helpers.test1_asset_id)
        p2 = portfolio1.get_position(helpers.test2_asset_id)
        assert(p1.units == -100)
        assert(p2.units == 100)
                
        portfolio1_nlv2 =  portfolio1_nlv1 + (-100 * (98-99)) + (100 * (102 - 101)) + (100 * (97-98)) + (-100 * (103 - 102))
        assert(portfolio1_nlv2 == portfolio1.get_nlv())
        
        portfolio2_nlv2 =  portfolio2_nlv1 + (50 * (98-99)) + (-50 * (102 - 101)) + (-50 * (97-98)) + (50 * (103 - 102))
        assert(portfolio2_nlv2 == portfolio2.get_nlv())
        assert(mp.get_nlv() == portfolio1_nlv2 + portfolio2_nlv2)
        
        
    def test_portfolio_target_allocations(self):
        return
        hydra = helpers.create_simple_hydra(logging=1)
        mp = hydra.get_master_portfolio()
        
        portfolio1 = hydra.new_portfolio("test_portfolio1",100000.0);
        portfolio2 = hydra.new_portfolio("test_portfolio2",100000.0);
        
                
        hydra.build()
        hydra.forward_pass()
        
        portfolio1.place_market_order(
            helpers.test2_asset_id,
            100.0,
            "dummy",
            FastTest.OrderExecutionType.EAGER,
            -1
        )
        
        hydra.on_open()
        hydra.backward_pass()
        
        hydra.forward_pass()
        hydra.on_open()
        
        allocations = {helpers.test1_asset_id : .4, helpers.test2_asset_id : .6}
        portfolio2.order_target_allocations(
            allocations,
            "dummy",
            .01,
        )
        hydra.backward_pass()
        
        p1 = portfolio2.get_position(helpers.test1_asset_id)
        p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(p1 is not None)
        assert(p2 is not None)
        assert(p1.units == (100000 * .4)/101)
        assert(p2.units == (100000 * .6)/99)
        
        mp_p1 = mp.get_position(helpers.test1_asset_id)
        mp_p2 = mp.get_position(helpers.test2_asset_id)
        assert(mp_p1 is not None)
        assert(mp_p2 is not None)
        assert(mp_p1.units == p1.units)
        assert(mp_p2.units == p2.units + 100)
        
        hydra.forward_pass()
        nlv = portfolio2.get_nlv()
        
        #2000-06-08 on open
        allocations = {helpers.test1_asset_id : .6, helpers.test2_asset_id : .4}
        portfolio2.order_target_allocations(
            allocations,
            "dummy",
            .0,
        )
        hydra.on_open()
        
        p1 = portfolio2.get_position(helpers.test1_asset_id)
        mp_p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(p1 is not None)
        assert(p2 is not None)
        assert(p2.units == (nlv * .4)/98)
        assert(p1.units == (nlv * .6)/102)
        
        hydra.backward_pass()
        hydra.forward_pass()
                
        allocations = {}
        portfolio2.order_target_allocations(
            allocations,
            "dummy",
            .0,
        )
        hydra.on_open()
        
        mp_p1 = portfolio2.get_position(helpers.test1_asset_id)
        mp_p2 = portfolio2.get_position(helpers.test2_asset_id)
        assert(mp_p1 is None)
        assert(mp_p2 is None)
        
        hydra.backward_pass()
                
        pl_1 = ((100000 * .4)/101)* (103-101)
        pl_2 = ((100000 * .6)/99) * (97 - 99)
        pl_2_total = 100000+ pl_1 + pl_2
        
        pl_1 = ((100000 * .4)/101)* (101-101) + ((100000 * .6)/101)* (101.5-101)
        pl_2 = ((100000 * .6)/99) * (101-99) + ((100000 * .4)/101)* (101.5-101)
        pl_3_total = 100000 + pl_1 + pl_2
        
        nlv_actual = np.array([200050,199800,pl_2_total, pl_3_total])
        print(nlv_actual)
        
        nlv_history1 = portfolio1.get_tracer(PortfolioTracerType.VALUE).get_nlv_history()  
        assert(np.array_equal(nlv_history1,np.array([100050,  99800,  99600, 100050.]))) 
        
        nlv_history2 = portfolio2.get_tracer(PortfolioTracerType.VALUE).get_nlv_history()  
        print(nlv_history2) 
          
            
    def test_portfolio_target_allocations_short(self):
        hydra = helpers.create_simple_hydra(logging=0)
        portfolio1 = hydra.new_portfolio("test_portfolio1",100000.0);

        hydra.build()
        
        hydra.forward_pass()
        hydra.on_open()
        hydra.backward_pass()
        
        hydra.forward_pass()
        hydra.on_open()
        
        allocations = {helpers.test1_asset_id : -.4, helpers.test2_asset_id : .6}
        portfolio1.order_target_allocations(
            allocations,
            "dummy",
            .01,
        )
        
        hydra.backward_pass()
                    

if __name__ == '__main__':
    unittest.main()
