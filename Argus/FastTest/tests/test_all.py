import unittest
from io import StringIO
from pprint import pprint

from test_asset import AssetTestMethods
from test_exchange import ExchangeTestMethods
from test_portfolio import PortfolioTestMethods

stream = StringIO()
runner = unittest.TextTestRunner(stream=stream)

result = runner.run(unittest.makeSuite(AssetTestMethods))
print('Tests run ', result.testsRun)
print('Errors ', result.errors)
pprint(result.failures)
stream.seek(0)
print('Test output\n', stream.read())

result = runner.run(unittest.makeSuite(ExchangeTestMethods))
print('Tests run ', result.testsRun)
print('Errors ', result.errors)
pprint(result.failures)
stream.seek(0)
print('Test output\n', stream.read())

result = runner.run(unittest.makeSuite(PortfolioTestMethods))
print('Tests run ', result.testsRun)
print('Errors ', result.errors)
pprint(result.failures)
stream.seek(0)
print('Test output\n', stream.read())