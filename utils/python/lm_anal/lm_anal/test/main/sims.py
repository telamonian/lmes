import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch.lm')

from lm_anal.src.main import Sims

import unittest

class SimsTestCase(unittest.TestCase):
    def setUp(self):
        self.sims = Sims(fPath=testDataPath)
    
    def loadData(self, full=False):
        pass
    
    def test_load_and_transform(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
#         self.loadData(full=True)
         
        self.sims[0].cook('OParamHists', src='BruteForce', tilingIDs=[1,2]) 
        
        oPVSum = np.sum(self.sims[0].opHists['sum'].order_parameter_values)
        oPVSumArr = self.sims[0].opHists['sum'].order_parameter_values
        intendedOPVSum = sum([np.sum(self.sims[0].opHists[i].order_parameter_values) for i in range(1,11)])
        intendedOPVSumArr = sum([self.sims[0].opHists[i].order_parameter_values for i in range(1,11)])
        
        # if everything==0, then none of these tests are very interesting
        self.assertTrue(oPVSum > 0)
        self.assertTrue(oPVSum==1010)
        self.assertTrue(oPVSum==intendedOPVSum)
        try:
            testBool = np.allclose(oPVSumArr, intendedOPVSumArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (oPVSumArr.tolist(), intendedOPVSumArr.tolist()))
