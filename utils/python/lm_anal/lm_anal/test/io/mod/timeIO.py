import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')

from lm_anal.src.io.mod import TimeIO

import unittest

class TimeIOTestCase(unittest.TestCase):
    def setUp(self):
        self.modIO = TimeIO(fPath=testDataPath)
        self.modIO.modFPath+='time'
        try:
            os.remove(self.modIO.modFPath)
        except FileNotFoundError:
            pass
        
    def tearDown(self):
        try:
            os.remove(self.modIO.modFPath)
        except FileNotFoundError:
            pass
        
    def loadData(self, full=False):
        pass
    
    def test_checkMod(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.modIO.saveMod()
        check = self.modIO.checkMod()
        self.assertTrue(check)
        
        self.modIO.modTime = 10
        self.modIO.wtf()
        check = self.modIO.checkMod()
        self.assertFalse(check)
        
    def test_saveMod(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.modIO.saveMod()
        
        with open(self.modIO.modFPath, 'r') as f:
            modTime = float(f.readline())
        intentendModTime = os.path.getmtime(testDataPath)
        self.assertEqual(modTime, intentendModTime)