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
    
    def test_load_plottable_useint_false(self):
        '''
        test that Sim can load a plottable without an intermediate file being present  
        '''
        ophID = 'ophTest'
        kwargs = {'tilingID' : 7}
        self.sim.InitPlottable(id=ophID, type='OParamProbabilityHist', useInt=False, **kwargs)
        
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
    
    def test_load_data(self):
        '''
        test that Sim creates the appropriate suite of data objects upon instantiation
        '''
        for dataObjectName in ['oparams', 'replicateTrajectories', 'tilings']:
            self.assertTrue(hasattr(self.sim, dataObjectName), msg='dataObject %s not in sim.__dict__: %s' % (dataObjectName, self.sim.__dict__))
            self.assertIn(dataObjectName, self.sim.dataDict, msg='dataObject %s not in sim.dataDict: %s' % (dataObjectName, self.sim.dataDict))