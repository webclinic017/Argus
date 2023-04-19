import unittest

# Discover all tests in the current directory
test_suite = unittest.defaultTestLoader.discover('.')

# Run the test suite
unittest.TextTestRunner().run(test_suite)