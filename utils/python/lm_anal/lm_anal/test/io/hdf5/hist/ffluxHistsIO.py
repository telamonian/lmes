import unittest
from lm_anal.test.datum.hist import FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase

class FFluxHistsHDF5IOTestCase(unittest.TestCase, FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase):
    def setUp(self):
        self.cleanUpInt()
        self.loadDataEagerly()
        
    def tearDown(self):
        pass
#         self.cleanUpInt()
        
    def test_point_of_origin(self):
        '''
        make sure that the point-of-origin is appropriate
        '''
        self.loadDataEagerly()
        self.assertEqual(self.ffluxHists.po, '.lmint')