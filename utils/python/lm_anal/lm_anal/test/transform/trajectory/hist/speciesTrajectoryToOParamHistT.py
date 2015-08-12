import unittest
from lm_anal.test.datum.hist import OParamHistsLazyTestBase, OParamHistsFieldsTestBase, OParamHistsMethodsTestBase

class SpeciesTrajectoryToOParamHistTTestCase(unittest.TestCase, OParamHistsLazyTestBase, OParamHistsFieldsTestBase, OParamHistsMethodsTestBase):
    def setUp(self):
        self.cleanUpInt()
        
    def tearDown(self):
        self.cleanUpInt()