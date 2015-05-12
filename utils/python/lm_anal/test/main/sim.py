import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.main.sim import Sim

import unittest

testLMPath = os.path.join(thisScriptDir, '../testData/biphasic_switch.lm')

class SimTestCase(unittest.TestCase):
    def setUp(self):
        self.sim = Sim(testLMPath)
    
    def test_load_data(self):
        '''
        test that Sim creates the appropriate suite of data objects upon instantiation
        '''
        for dataObjectName in ['oparams', 'replicateTrajectories', 'tilings']:
            self.assertTrue(hasattr(self.sim, dataObjectName), msg='dataObject %s not in sim.__dict__: %s' % (dataObjectName, self.sim.__dict__))
            self.assertIn(dataObjectName, self.sim.dataDict, msg='dataObject %s not in sim.dataDict: %s' % (dataObjectName, self.sim.dataDict))

class SimCreateOParamHistTestCase(unittest.TestCase):
    def setUp(self):
        self.sim = Sim(testLMPath)
    
    def createdOParamTest(self, ophID):
        rank = self.sim.oparamProbabilityHists[ophID].rank
        intendedRank = 1
        self.assertEqual(rank, intendedRank)
        
        dims = np.array(self.sim.oparamProbabilityHists[ophID].dims)
        intendedDims = np.array([12])
        self.assertTrue(np.allclose(dims, intendedDims), msg='%s is not allclose to %s' % (dims.tolist(), intendedDims.tolist()))
        
        edges = np.array(self.sim.oparamProbabilityHists[ophID].edges)
        intendedEdges = np.array([-25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 15.0, 20.0, 25.0])
        self.assertTrue(np.allclose(edges, intendedEdges), msg='%s is not allclose to %s' % (edges.tolist(), intendedEdges.tolist()))
        
        vals = np.array(self.sim.oparamProbabilityHists[ophID].vals)
        intendedVals = np.array([50.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 7.0, 63.0, 887.0])
        self.assertTrue(np.allclose(vals, intendedVals), msg='%s is not allclose to %s' % (vals.tolist(), intendedVals.tolist()))
        
    def test_load_plottable_useint_false(self):
        '''
        test that Sim can load a plottable without an intermediate file being present  
        '''
        ophID = 'ophTest'
        kwargs = {'tilingID' : 7}
        self.sim.InitPlottable(id=ophID, type='OParamProbabilityHist', useInt=False, **kwargs)
        
        self.createdOParamTest(ophID=ophID)
    
    def test_load_plottable_useint_true_without_existing_lmint(self):
        '''
        test that Sim can load a plottable from an intermediate file
        '''
        try:
            os.remove(self.sim.intermediatePath)
        except FileNotFoundError:
            pass
        ophID = 'ophTestWithRed'
        kwargs = {'tilingID' : 7}
        self.sim.InitPlottable(id=ophID, type='OParamProbabilityHist', useInt=True, **kwargs)
        
        self.createdOParamTest(ophID=ophID)
    
    def test_load_plottable_useint_true_with_existing_lmint(self):
        '''
        test that Sim can load a plottable from an intermediate file
        '''
        try:
            os.remove(self.sim.intermediatePath)
        except FileNotFoundError:
            pass
        ophID = 'ophTestWithRed'
        kwargs = {'tilingID' : 7}
        # write the histogram out to the lmint file, then delete the in-memory representation
        self.sim.InitPlottable(id=ophID, type='OParamProbabilityHist', useInt=True, **kwargs)
        del self.sim
        # reload the histogram from the lmint file. Notice the lack of **kwargs in the InitPlottable signature, since it no longer needs the tilingID argument
        self.sim = Sim(testLMPath)
        self.sim.InitPlottable(id=ophID, type='OParamProbabilityHist', useInt=True)
        
        self.createdOParamTest(ophID=ophID)
        
class SimCreateOParamTrajectoryTestCase(unittest.TestCase):
    def setUp(self):
        self.sim = Sim(testLMPath)
    
    def createdOParamTrajectoryTest(self, otID):
        numberOP = self.sim.oparamTrajectories[otID].number_order_parameters
        self.assertEqual(numberOP, 1)
        
        numberEntries = self.sim.oparamTrajectories[otID].number_entries
        self.assertEqual(numberEntries, 101)
        
        orderParameterValuesArr = np.array(self.sim.oparamTrajectories[otID].order_parameter_values).flatten()
        intendedOrderParameterValuesArr = np.array([24.0,39.0,34.0,35.0,40.0,45.0,33.0,33.0,35.0,31.0,48.0,45.0,43.0,61.0,47.0,50.0,38.0,33.0,33.0,39.0,39.0,39.0,41.0,41.0,36.0,41.0,38.0,40.0,54.0,37.0,32.0,38.0,32.0,34.0,49.0,52.0,33.0,49.0,49.0,39.0,47.0,42.0,45.0,57.0,33.0,40.0,34.0,46.0,-25.0,-34.0,-35.0,-51.0,-32.0,-44.0,-38.0,-34.0,-37.0,-37.0,-68.0,-37.0,-36.0,-41.0,-40.0,-44.0,-39.0,-38.0,-40.0,-36.0,-49.0,-38.0,-44.0,-48.0,-29.0,-37.0,-19.0,-36.0,-45.0,-36.0,-38.0,-36.0,-35.0,-36.0,-29.0,-27.0,-39.0,-48.0,-41.0,-41.0,-43.0,-46.0,-46.0,-22.0,-37.0,-41.0,-29.0,-38.0,-40.0,-37.0,-35.0,-41.0,-42.0])
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValuesArr))
    
        timeArr = np.array(self.sim.oparamTrajectories[otID].time).flatten()
        intendedTimeArr = np.array([0,1000,2000,3000,4000,5000,6000,7000,8000,9000,10000,11000,12000,13000,14000,15000,16000,17000,18000,19000,20000,21000,22000,23000,24000,25000,26000,27000,28000,29000,30000,31000,32000,33000,34000,35000,36000,37000,38000,39000,40000,41000,42000,43000,44000,45000,46000,47000,48000,49000,50000,51000,52000,53000,54000,55000,56000,57000,58000,59000,60000,61000,62000,63000,64000,65000,66000,67000,68000,69000,70000,71000,72000,73000,74000,75000,76000,77000,78000,79000,80000,81000,82000,83000,84000,85000,86000,87000,88000,89000,90000,91000,92000,93000,94000,95000,96000,97000,98000,99000,100000])
        self.assertTrue(np.allclose(timeArr, intendedTimeArr))
    
        idInt = self.sim.oparamTrajectories[otID].trajectory_id
        self.assertEqual(idInt, 5)
    
    def test_load_oparam_trajectory_useint_false(self):
        '''
        test that Sim can load a plottable without an intermediate file being present  
        '''
        otID = 'otTest'
        kwargs = {'oparamID': 0,
                  'trajID': 5}
        self.sim.InitPlottable(id=otID, type='OParamTrajectory', useInt=False, **kwargs)
          
        self.createdOParamTrajectoryTest(otID)
#         rank = self.sim.oparamProbabilityHists[ophID].rank
#         intendedRank = 1
#         self.assertEqual(rank, intendedRank)
#           
#         dims = np.array(self.sim.oparamProbabilityHists[ophID].dims)
#         intendedDims = np.array([12])
#         self.assertTrue(np.allclose(dims, intendedDims), msg='%s is not allclose to %s' % (dims.tolist(), intendedDims.tolist()))
#           
#         edges = np.array(self.sim.oparamProbabilityHists[ophID].edges)
#         intendedEdges = np.array([-25.0, -20.0, -15.0, -10.0, -5.0, 0.0, 5.0, 10.0, 15.0, 20.0, 25.0])
#         self.assertTrue(np.allclose(edges, intendedEdges), msg='%s is not allclose to %s' % (edges.tolist(), intendedEdges.tolist()))
#           
#         vals = np.array(self.sim.oparamProbabilityHists[ophID].vals)
#         intendedVals = np.array([50.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 7.0, 63.0, 887.0])
#         self.assertTrue(np.allclose(vals, intendedVals), msg='%s is not allclose to %s' % (vals.tolist(), intendedVals.tolist()))
