import sys
import os
import unittest

sys.path.append(os.path.abspath('..'))

import FastTest
import Asset
import helpers

class MyTestCase(unittest.TestCase):
    def test_something(self):
        asset = helpers.load_asset(
            helpers.test1_file_path,
            helpers.test1_asset_id
        )
        print(asset)

        self.assertEqual(True, True)  # add assertion here


if __name__ == '__main__':
    unittest.main()
