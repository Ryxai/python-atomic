import unittest

test_modules = [
    "integer_tests",
    "markable_reference_tests",
    "reference_tests"
    ]

test_suite = unittest.TestSuite()

for test_module in test_modules:
    test_suite.addTest(unittest.defaultTestLoader.
    loadTestsFromNames(test_module))
    
unittest.TextTestRunner().run(test_suite)