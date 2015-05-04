import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import numpy as np
np.random.seed(0)
from scipy import stats
import src
from src.oparam.oparams import OParams
from src.plottable.oparamProbabilityHist import OParamProbabilityHist
from src.replicate.replicateTrajectories import ReplicateTrajectories
from src.tiling.tilings import Tilings

import unittest

class OParamProbabilityHistTestCase(unittest.TestCase):
    testDataPath = '../testData/biphasic_switch.lm'
    
    def setUp(self):
        self.oparams = OParams(fPath=os.path.join(thisScriptDir, self.__class__.testDataPath))
        self.oparams.rffHDF5()
        self.tilings = Tilings(fPath=os.path.join(thisScriptDir, self.__class__.testDataPath))
        self.tilings.rffHDF5()
        self.replicateTrajectories = ReplicateTrajectories(fPath=os.path.join(thisScriptDir, self.__class__.testDataPath))
        self.replicateTrajectories.rffHDF5(full=True)
        self.hist = OParamProbabilityHist(self, tilingID=7)
    
    def test_dims(self):
        '''
        test dims field
        '''
        dims = np.array(self.hist.dims)
        intendedDims = np.array((12,))
        self.assertTrue(np.allclose(dims, intendedDims), msg='%s is not allclose to %s' % (dims, intendedDims))
    
    def test_edges(self):
        '''
        test edges
        '''
        edges = self.hist.GetEdges()
        intendedEdges = []
        intendedEdges.append(np.linspace(-25,25,self.hist.rDims[0]))
        self.assertTrue(np.allclose(edges, np.array(intendedEdges)), msg='%s is not allclose to %s' % (edges, intendedEdges))
    
    def test_rank(self):
        '''
        test rank field
        '''
        rank = self.hist.rank
        intendedRank = 1
        self.assertEqual(rank, intendedRank)
    
    def test_AddSpeciesCounts(self):
        '''
        test reading of times from hdf5 files
        '''
        self.hist.AddSpeciesCounts(self.replicateTrajectories[5].species_count)
        vals = np.array(self.hist.vals)
        intendedVals = np.array([50.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 47.0])
        self.assertTrue(np.allclose(vals, intendedVals), msg='%s is not allclose to %s' % (vals.tolist(), intendedVals.tolist()))
        
    def test_KBDiv(self):
        '''
        tests the determination of the Kullbeck Liebler divergence
        '''
        