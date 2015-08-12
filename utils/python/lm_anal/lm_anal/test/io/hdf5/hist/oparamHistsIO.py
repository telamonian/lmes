import unittest
from lm_anal.test.datum.hist import OParamHistsLazyTestBase, OParamHistsFieldsTestBase, OParamHistsMethodsTestBase

class OParamHistsHDF5IOTestCase(unittest.TestCase, OParamHistsLazyTestBase, OParamHistsFieldsTestBase):
    def setUp(self):
        self.cleanUpInt()
        self.loadDataEagerly()
        
    def tearDown(self):
        pass
        self.cleanUpInt()
        
    def test_point_of_origin(self):
        '''
        make sure that the point-of-origin is appropriate
        '''
        self.loadDataEagerly()
        self.assertEqual(self.opHists.po, '.lmint')