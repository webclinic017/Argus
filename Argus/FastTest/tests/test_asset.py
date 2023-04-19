import sys
import os
import unittest

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset
import helpers
import time

class AssetTestMethods(unittest.TestCase):
    def test_asset_load(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )
        assert(True);

    def test_asset_get(self):
        asset1 = helpers.load_asset(
            helpers.test1_file_path,
            "asset1"
        )
        assert(asset1.get("CLOSE",0) == 101)
        assert(asset1.get("OPEN",3) == 105)

if __name__ == '__main__':
    unittest.main()
