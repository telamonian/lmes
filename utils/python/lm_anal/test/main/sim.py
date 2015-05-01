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
    
    def test_load_data(self):
        '''
        test that Sim creates the appropriate suite of data objects upon instantiation
        '''
        for dataObjectName in ['oparams', 'replicateTrajectories', 'tilings']:
            self.assertTrue(hasattr(self.sim, dataObjectName), msg='dataObject %s not in sim.__dict__: %s' % (dataObjectName, self.sim.__dict__))
            self.assertIn(dataObjectName, self.sim.dataDict, msg='dataObject %s not in sim.dataDict: %s' % (dataObjectName, self.sim.dataDict))