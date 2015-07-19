import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch.lm')

from lm_anal.src.main import Sims

import unittest

class SimsTestCase(unittest.TestCase):
    def __init__(self):
        pass
    
    def setUp(self):
        self.sims = Sims(fPath=testDataPath)
    
    def loadData(self, full=False):
        self.sims.get('OParamHists', src='FFluxOutputs', full=full)
    
    def test_order_parameter_values(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.loadData(full=True)
        
        opVArr = np.array(self.opHists[19].order_parameter_values)
        print(opVArr.tolist())
        print(opVArr.sum())
        intendedOPVArr = np.zeros(opVArr.shape)