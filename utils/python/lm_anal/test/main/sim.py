import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.main.sim import Sim

import unittest

class SimTestCase(unittest.TestCase):
    def setUp(self):
        self.sim = Sim(os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))
    
    def test_load_plottable_without_lmint(self):
        '''
        test that Sim can load a plottable without an intermediate file being present  
        '''
        kwargs = {'tilingID' : 7}
        self.sim.InitPlottable(id=0, type='OParamProbabilityHist', useInt=False, **kwargs)
        self.sim.plottables[0].LoadData()
        vals = np.array(self.sim.plottables[0].vals)
        intendedVals = np.array([50.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 7.0, 63.0, 887.0])
        self.assertTrue(np.allclose(vals, intendedVals), msg='%s is not allclose to %s' % (vals.tolist(), intendedVals.tolist()))
    
    def test_load_plottable_with_lmint(self):
        '''
        test that Sim can load a plottable from an intermediate file
        '''
        pass
    
    def test_load_data(self):
        '''
        test that Sim creates the appropriate suite of data objects upon instantiation
        '''
        for dataObjectName in ['oparams', 'replicateTrajectories', 'tilings']:
            self.assertTrue(hasattr(self.sim, dataObjectName), msg='dataObject %s not in sim.__dict__: %s' % (dataObjectName, self.sim.__dict__))
            self.assertIn(dataObjectName, self.sim.dataDict, msg='dataObject %s not in sim.dataDict: %s' % (dataObjectName, self.sim.dataDict))