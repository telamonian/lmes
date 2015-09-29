import os
from pathlib import Path

from lm_anal.test.datum.trajectory import OParamTrajectoriesSimTestBase, OParamTrajectoriesFieldsTestSet, OParamTrajectoriesClassTestSet

thisScriptDir = Path(__file__).resolve().parent
testFilePath = (thisScriptDir / Path('../../../testData/biphasic_switch.lm')).resolve()

class OParamTrajectoriesHDF5IOTestBase(OParamTrajectoriesFieldsTestSet, OParamTrajectoriesClassTestSet):
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
        self.assertEqual(self.oparamTrajectories.po, '.lmint')
    
class OParamTrajectoriesHDF5IOSimTestBase(OParamTrajectoriesHDF5IOTestBase, OParamTrajectoriesSimTestBase):
    pass
