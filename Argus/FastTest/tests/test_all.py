import unittest
from io import StringIO
from pprint import pprint

from test_asset import AssetTestMethods


stream = StringIO()
runner = unittest.TextTestRunner(stream=stream)

result = runner.run(unittest.makeSuite(AssetTestMethods))
print('Tests run ', result.testsRun)
print('Errors ', result.errors)
pprint(result.failures)
stream.seek(0)
print('Test output\n', stream.read())