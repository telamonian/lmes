import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')

from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.datum.parameter import SimulationParameters

class SimulationParametersTestCase(unittest.TestCase):
    def setUp(self):
        self.simParamsIO = SimulationParametersIO(fPath=testDataPath)
        
        self.simParams = SimulationParameters()
    
    def loadData(self, full=False):
        self.simParamsIO.rff(container=self.simParams, full=full)
        
    def test_keys_from_hdf5(self):
        '''
        test the edge_id field in FFluxTrajectory
        '''
        self.loadData()
        
        keyArr = np.array(self.simParams[0].key)
        intendedKeyArr = np.array(['crossingsPerPhase', 'maxTime', 'maxPhaseZeroTime', 'maxSteps', 'maxWorkUnitSteps', 'writeInterval'])
        self.assertTrue(np.all(keyArr==intendedKeyArr), 
                        msg='%s is not allclose to %s' % (keyArr.tolist(), intendedKeyArr.tolist()))
        
    def test_values_from_hdf5(self):
        '''
        test the edge_id field in FFluxTrajectory
        '''
        self.loadData()
        
        valueArr = np.array(self.simParams[0].value)
        intendedValueArr = np.array(['100 ', '1e5 ', '10000 ', '10000000000 ', '10000000 ', '1e3 '])
        self.assertTrue(np.all(valueArr==intendedValueArr), 
                        msg='%s is not allclose to %s' % (valueArr.tolist(), intendedValueArr.tolist()))