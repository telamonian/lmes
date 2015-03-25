import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../..'))

import numpy as np
np.random.seed(0)
import src
from src.plot.plotables.hist import Hist

import unittest

class HistTestCase(unittest.TestCase):
    def setUp(self):
        dims = np.array((7,6,9))
        boundedness = np.array(((0,0),(0,0),(0,0)))
        edges = []
        edges+=np.linspace(-25,25,dims[0]-1).tolist()
        edges+=np.linspace(-30,30,dims[1]-1).tolist()
        edges+=np.linspace(-15,15,dims[2]-1).tolist()
        self.hist = Hist(boundedness=boundedness,dims=dims,edges=edges,rank=3)
        data = np.random.rand(np.prod(dims),3)                                                                                \
              *np.array((50 + 2*float(50)/(dims[0] - 2), 60 + 2*float(60)/(dims[1] - 2), 30 + 2*float(30)/(dims[2] - 2)))     \
              -np.array((25 + float(50)/(dims[0] - 2), 30 + float(60)/(dims[1] - 2), 15 + float(30)/(dims[2] - 2)))
        self.hist.AddData(data)
#         self.replicateTrajectories = ReplicateTrajectories(fPath=os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))
#         self.replicateTrajectories.rffHDF5()
#         self.replicateTrajectories.InitOParamProbabilityHist((1,2))
#         self.hist = self.replicateTrajectories.GetOParamProbabilityHist((1.2))
        
    def test_boundedness(self):
        '''
        test boundedness
        '''
        boundedness = np.array(self.hist.boundedness)
        intendedBoundedness = np.array(((0,0),(0,0),(0,0)))
        self.assertTrue(np.allclose(boundedness, intendedBoundedness.flatten()))
    
    def test_dims(self):
        '''
        test dims field
        '''
        dims = np.array(self.hist.dims)
        intendedDims = np.array((7,6,9))
        self.assertTrue(np.allclose(dims, intendedDims))
    
    def test_edges(self):
        '''
        test edges
        '''
        edges = np.array(self.hist.edges)
        intendedEdges = []
        intendedEdges+=np.linspace(-25,25,self.hist.dims[0]-1).tolist()
        intendedEdges+=np.linspace(-30,30,self.hist.dims[1]-1).tolist()
        intendedEdges+=np.linspace(-15,15,self.hist.dims[2]-1).tolist()
        self.assertTrue(np.allclose(edges, np.array(intendedEdges)))
    
    def test_find_index(self):
        '''
        test the find index function
        '''
        datum = np.array((-12,3,14))
        index = self.hist.FindTileIndex(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*2 + np.prod(self.hist.dims[2:self.hist.rank])*3 + 7
        self.assertEqual(index, intendedIndex)
        
        datum = np.array((25,30,15))
        index = self.hist.FindTileIndex(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*6 + np.prod(self.hist.dims[2:self.hist.rank])*5 + 8
        self.assertEqual(index, intendedIndex)
        
        datum = np.array((24.999999,29.999999,14.999999))
        index = self.hist.FindTileIndex(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*5 + np.prod(self.hist.dims[2:self.hist.rank])*4 + 7
        self.assertEqual(index, intendedIndex)
        
    def test_find_indices(self):
        '''
        test the find indices function
        '''
        datum = np.array((-12,3,14))
        indices = self.hist.FindTileIndices(datum)
        intendedIndices = np.array((2,3,7))
        self.assertTrue(np.allclose(indices, intendedIndices))
        
        datum = np.array((25,30,15))
        indices = self.hist.FindTileIndices(datum)
        intendedIndices = np.array((6,5,8))
        self.assertTrue(np.allclose(indices, intendedIndices))
        
        datum = np.array((24.999999,29.999999,14.999999))
        indices = self.hist.FindTileIndices(datum)
        intendedIndices = np.array((5,4,7))
        self.assertTrue(np.allclose(indices, intendedIndices))
    
    def test_rank(self):
        '''
        test rank field
        '''
        rank = self.hist.rank
        self.assertEqual(rank, 3)
    
    def test_vals(self):
        '''
        test reading of times from hdf5 files
        '''
        vals = np.array(self.hist.vals)
        intendedVals = np.array([2.0, 0.0, 0.0, 1.0, 1.0, 1.0, 3.0, 1.0, 2.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 2.0, 3.0, 3.0, 3.0, 0.0, 0.0, 2.0, 1.0, 4.0, 1.0, 2.0, 4.0, 3.0, 1.0, 2.0, 0.0, 0.0, 1.0, 1.0, 2.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 4.0, 1.0, 0.0, 2.0, 1.0, 0.0, 2.0, 2.0, 3.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 4.0, 1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 2.0, 1.0, 0.0, 1.0, 1.0, 2.0, 1.0, 2.0, 1.0, 1.0, 1.0, 4.0, 1.0, 1.0, 0.0, 2.0, 2.0, 4.0, 1.0, 1.0, 2.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 2.0, 0.0, 1.0, 1.0, 0.0, 0.0, 2.0, 0.0, 1.0, 0.0, 2.0, 1.0, 0.0, 2.0, 1.0, 1.0, 2.0, 0.0, 2.0, 1.0, 2.0, 1.0, 0.0, 1.0, 0.0, 2.0, 1.0, 1.0, 2.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 1.0, 2.0, 3.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 1.0, 1.0, 3.0, 4.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 3.0, 0.0, 0.0, 1.0, 1.0, 2.0, 1.0, 1.0, 0.0, 1.0, 3.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 1.0, 0.0, 3.0, 0.0, 2.0, 1.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 1.0, 2.0, 0.0, 1.0, 2.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 3.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 3.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 2.0, 3.0, 0.0, 0.0, 3.0, 1.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 1.0, 4.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 3.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 1.0, 4.0, 2.0, 0.0, 2.0, 1.0, 0.0, 1.0, 1.0, 3.0, 1.0, 2.0, 0.0, 0.0, 2.0, 0.0, 3.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 4.0, 4.0, 0.0, 3.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 1.0, 2.0, 0.0, 1.0, 1.0, 0.0, 3.0, 2.0, 1.0, 0.0, 1.0, 1.0, 0.0, 2.0, 0.0, 5.0, 1.0, 3.0, 1.0, 2.0, 3.0, 0.0, 0.0])
        self.assertTrue(np.allclose(vals, intendedVals))