import os
from pathlib import Path

from lm_anal.test.datum.hist import FFluxHistsFieldsTestBase, FFluxHistsSimTestBase

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
testFilePath = (thisScriptDir / Path('../../../testData/biphasic_switch.lm')).resolve()
# testFilePath = (thisScriptDir / Path('../../../../../../../../regression/biphasic_switch.lm')).resolve()

class FFluxHistsHDF5IOTestBase(FFluxHistsFieldsTestBase):
    testFilePath = testFilePath
    
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
    
class FFluxHistsHDF5IOSimTestBase(FFluxHistsHDF5IOTestBase, FFluxHistsSimTestBase):
    pass

# import unittest
# from lm_anal.test.datum.hist import FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase
# 
# class FFluxHistsHDF5IOTestCase(unittest.TestCase, FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase):
#     def setUp(self):
#         self.cleanUpInt()
#         self.loadDataEagerly()
#         
#     def tearDown(self):
#         pass
# #         self.cleanUpInt()
#         
#     def test_point_of_origin(self):
#         '''
#         make sure that the point-of-origin is appropriate
#         '''
#         self.loadDataEagerly()
#         self.assertEqual(self.ffluxHists.po, '.lmint')