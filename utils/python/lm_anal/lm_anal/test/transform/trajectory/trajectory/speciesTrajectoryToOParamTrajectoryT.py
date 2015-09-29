import os
from pathlib import Path

from lm_anal.test.datum.trajectory import OParamTrajectoriesSimTestBase, OParamTrajectoriesClassTestSet, OParamTrajectoriesFieldsTestSet

thisScriptDir = Path(__file__).resolve().parent
testFilePath = (thisScriptDir / Path('../../../testData/biphasic_switch.lm')).resolve()

class SpeciesTrajectoryToOParamTrajectoryTTestSet(OParamTrajectoriesClassTestSet, OParamTrajectoriesFieldsTestSet):
    testFilePath = testFilePath
    
#     def setUp(self):
#         self.cleanUpInt()
#         self.loadDataEagerly()
#           
#     def tearDown(self):
#         pass
# #         self.cleanUpInt()
    
class SpeciesTrajectoryToOParamTrajectoryTSimTestBase(SpeciesTrajectoryToOParamTrajectoryTTestSet, OParamTrajectoriesSimTestBase):
    pass

