import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import numpy as np
np.random.seed(0)
from scipy import stats
import src
from lm_anal.src.plottable.hist import Hist

import unittest

class HistTestCase(unittest.TestCase):
    def setUp(self):
        dims = np.array((7,6,9))
        edges = []
        edges+=np.linspace(-25,25,dims[0]-1).tolist()
        edges+=np.linspace(-30,30,dims[1]-1).tolist()
        edges+=np.linspace(-15,15,dims[2]-1).tolist()
        self.hist = Hist(dims=dims,edges=edges,rank=3)
        data = np.random.rand(np.prod(dims),3)                                                                                \
              *np.array((50 + 2*float(50)/(dims[0] - 2), 60 + 2*float(60)/(dims[1] - 2), 30 + 2*float(30)/(dims[2] - 2)))     \
              -np.array((25 + float(50)/(dims[0] - 2), 30 + float(60)/(dims[1] - 2), 15 + float(30)/(dims[2] - 2)))
        self.hist.AddObs(data)
    
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
        index = self.hist.FindTileIndex_Buf(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*2 + np.prod(self.hist.dims[2:self.hist.rank])*3 + 7
        self.assertEqual(index, intendedIndex)
        
        datum = np.array((25,30,15))
        index = self.hist.FindTileIndex_Buf(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*6 + np.prod(self.hist.dims[2:self.hist.rank])*5 + 8
        self.assertEqual(index, intendedIndex)
        
        datum = np.array((24.999999,29.999999,14.999999))
        index = self.hist.FindTileIndex_Buf(datum)
        intendedIndex = np.prod(self.hist.dims[1:self.hist.rank])*5 + np.prod(self.hist.dims[2:self.hist.rank])*4 + 7
        self.assertEqual(index, intendedIndex)
        
    def test_find_indices(self):
        '''
        test the find indices function
        '''
        datum = np.array((-12,3,14))
        indices = self.hist.FindTileIndices_Buf(datum)
        intendedIndices = np.array((2,3,7))
        self.assertTrue(np.allclose(indices, intendedIndices))
        
        datum = np.array((25,30,15))
        indices = self.hist.FindTileIndices_Buf(datum)
        intendedIndices = np.array((6,5,8))
        self.assertTrue(np.allclose(indices, intendedIndices))
        
        datum = np.array((24.999999,29.999999,14.999999))
        indices = self.hist.FindTileIndices_Buf(datum)
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
        dims = self.hist.dims
        data = np.random.rand(np.prod(dims)*10,3)                                                                             \
              *np.array((50 + 2*float(50)/(dims[0] - 2), 60 + 2*float(60)/(dims[1] - 2), 30 + 2*float(30)/(dims[2] - 2)))     \
              -np.array((25 + float(50)/(dims[0] - 2), 30 + float(60)/(dims[1] - 2), 15 + float(30)/(dims[2] - 2)))
        self.hist.SetObs(data)
        vals = np.array(self.hist.vals)
        intendedVals = np.array([[[8.0, 9.0, 12.0, 18.0, 13.0, 9.0, 13.0, 8.0, 13.0], [9.0, 11.0, 10.0, 8.0, 12.0, 7.0, 7.0, 11.0, 13.0], [12.0, 11.0, 12.0, 11.0, 14.0, 9.0, 7.0, 9.0, 7.0], [9.0, 19.0, 7.0, 11.0, 14.0, 12.0, 7.0, 9.0, 7.0], [10.0, 6.0, 13.0, 12.0, 9.0, 11.0, 11.0, 10.0, 13.0], [9.0, 12.0, 6.0, 16.0, 12.0, 6.0, 7.0, 6.0, 11.0]], [[13.0, 7.0, 9.0, 6.0, 11.0, 8.0, 9.0, 4.0, 15.0], [16.0, 10.0, 9.0, 10.0, 11.0, 13.0, 9.0, 10.0, 10.0], [12.0, 9.0, 8.0, 11.0, 12.0, 9.0, 4.0, 10.0, 15.0], [11.0, 7.0, 12.0, 14.0, 7.0, 8.0, 5.0, 5.0, 7.0], [10.0, 6.0, 6.0, 10.0, 10.0, 4.0, 9.0, 7.0, 16.0], [17.0, 4.0, 10.0, 10.0, 11.0, 10.0, 3.0, 12.0, 6.0]], [[12.0, 15.0, 15.0, 10.0, 9.0, 15.0, 5.0, 12.0, 9.0], [10.0, 10.0, 10.0, 10.0, 11.0, 8.0, 13.0, 9.0, 7.0], [9.0, 13.0, 6.0, 7.0, 12.0, 7.0, 9.0, 8.0, 6.0], [9.0, 11.0, 5.0, 7.0, 13.0, 7.0, 7.0, 13.0, 10.0], [15.0, 14.0, 10.0, 9.0, 7.0, 12.0, 8.0, 6.0, 8.0], [5.0, 10.0, 9.0, 11.0, 16.0, 10.0, 14.0, 7.0, 8.0]], [[14.0, 13.0, 16.0, 11.0, 5.0, 11.0, 10.0, 13.0, 15.0], [12.0, 7.0, 13.0, 9.0, 8.0, 5.0, 14.0, 8.0, 7.0], [11.0, 13.0, 10.0, 9.0, 12.0, 7.0, 16.0, 5.0, 16.0], [6.0, 10.0, 12.0, 15.0, 11.0, 13.0, 5.0, 12.0, 17.0], [9.0, 10.0, 7.0, 8.0, 10.0, 11.0, 8.0, 10.0, 16.0], [8.0, 8.0, 10.0, 8.0, 11.0, 7.0, 12.0, 9.0, 15.0]], [[8.0, 10.0, 16.0, 13.0, 7.0, 7.0, 12.0, 13.0, 10.0], [11.0, 9.0, 13.0, 8.0, 10.0, 10.0, 7.0, 8.0, 6.0], [17.0, 7.0, 14.0, 8.0, 8.0, 11.0, 9.0, 10.0, 14.0], [8.0, 7.0, 5.0, 9.0, 9.0, 12.0, 5.0, 13.0, 14.0], [10.0, 9.0, 5.0, 10.0, 9.0, 4.0, 6.0, 9.0, 12.0], [9.0, 3.0, 10.0, 12.0, 9.0, 8.0, 10.0, 9.0, 12.0]], [[8.0, 13.0, 7.0, 5.0, 14.0, 8.0, 10.0, 17.0, 8.0], [9.0, 7.0, 9.0, 12.0, 10.0, 15.0, 11.0, 8.0, 7.0], [12.0, 15.0, 11.0, 9.0, 11.0, 12.0, 13.0, 14.0, 4.0], [11.0, 12.0, 13.0, 9.0, 6.0, 14.0, 7.0, 11.0, 13.0], [10.0, 12.0, 7.0, 8.0, 8.0, 8.0, 11.0, 11.0, 10.0], [15.0, 7.0, 12.0, 14.0, 10.0, 7.0, 9.0, 9.0, 8.0]], [[11.0, 11.0, 10.0, 7.0, 16.0, 10.0, 11.0, 9.0, 6.0], [12.0, 15.0, 8.0, 11.0, 12.0, 8.0, 9.0, 11.0, 9.0], [8.0, 10.0, 7.0, 16.0, 13.0, 17.0, 9.0, 15.0, 9.0], [16.0, 10.0, 11.0, 12.0, 5.0, 11.0, 8.0, 9.0, 11.0], [12.0, 15.0, 6.0, 12.0, 9.0, 5.0, 8.0, 5.0, 6.0], [10.0, 14.0, 7.0, 10.0, 13.0, 8.0, 11.0, 12.0, 8.0]]])
        # chisquare test for uniformity of resulting histogram
        self.assertGreaterEqual(stats.chisquare(vals.ravel())[1], .8)
        self.assertTrue(np.allclose(vals, intendedVals), msg='%s is not allclose to %s' % (vals.tolist(), intendedVals.tolist()))