import h5py
import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch.lm')

from lm_anal.src.main import Sims

import unittest

class SimsBFTrajToOPHistTestCase(unittest.TestCase):
    '''
    test how the Sims class handles the functions related to loading brute force trajectories, transforming them into 
    '''
    def setUp(self):
        self.sims = Sims(fPath=testDataPath)
        # make sure all of the .lmint/.mod stuff is cleaned up
        try:
            os.remove(self.sims[0].modIO.modFPath)
        except FileNotFoundError:
            pass
        try:
            os.remove(self.sims[0].intermediatePath)
        except FileNotFoundError:
            pass
    
    def tearDown(self):
        pass
        # clean up all of the .lmint/.mod stuff
#         try:
#             os.remove(self.sims[0].modIO.modFPath)
#         except FileNotFoundError:
#             pass
#         try:
#             os.remove(self.sims[0].intermediatePath)
#         except FileNotFoundError:
#             pass
    
    def checkSumHist(self, binSum):
        oPVSum = np.sum(self.sims[0].opHists['sum'].order_parameter_values)
        oPVSumArr = self.sims[0].opHists['sum'].order_parameter_values
        intendedOPVSum = sum([np.sum(self.sims[0].opHists[i].order_parameter_values) for i in range(1,11)])
        intendedOPVSumArr = sum([self.sims[0].opHists[i].order_parameter_values for i in range(1,11)])
        
        # if everything==0, then none of these tests are very interesting
        self.assertTrue(oPVSum > 0)
        self.assertEqual(oPVSum, binSum)
#         self.assertEqual(oPVSum, intendedOPVSum)
        try:
            testBool = np.allclose(oPVSumArr, intendedOPVSumArr)
        except ValueError:
            testBool = False
#         self.assertTrue(testBool, msg='not allclose: %s\n%s' % (oPVSumArr.tolist(), intendedOPVSumArr.tolist()))
    
    def loadData(self, full=False):
        pass
    
#     def test_rff_and_transform(self):
#         '''
#         test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
#         '''
# #         self.loadData(full=True)
#          
#         self.sims[0].gen('OParamHists', src='BruteForce', tilingIDs=[1,2], readInt=False, writeInt=False) 
#         self.checkSumHist(binSum=1010)

    def test_rff_close_rff_from_lmint(self):
        '''
        test that the order_parameter_values in the histogram that results from doing FFluxTrajectoryToOParamHistT transform, and the histogram that results from reading in from the .lmint created during the transform, match
        '''
#         self.loadData(full=True)
         
        self.sims[0].gen('OParamHists', src='BruteForce', tilingIDs=[1,2]) 
        # clear out all of the data that's been collected into the Sim object
        self.sims[0].clear()
        # alter the histogram in the .lmint file
        with h5py.File(self.sims[0].intermediatePath, 'a') as f:
            f['OParamHists/sum/h_raw'][99,99] = 2317
        # reload the histogram, this time using the data in the .lmint file, and see if matches with your altered expectations
        self.sims[0].gen('OParamHists', src='BruteForce', tilingIDs=[1,2])
        self.checkSumHist(binSum=(1010 + 2317))
