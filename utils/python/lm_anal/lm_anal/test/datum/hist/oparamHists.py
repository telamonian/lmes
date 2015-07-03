import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')

from .groundTruths import intendedOrderParameterValues5Arr
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO
from lm_anal.src.datum.hist import OParamHists
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories
from lm_anal.src.transform import Transforms

import unittest

class OParamHistsTestCase(unittest.TestCase):
    def setUp(self):
        self.bfTrajsIO = BruteForceTrajectoriesIO(fPath=testDataPath)
        self.oparamsIO = OParamsIO(fPath=testDataPath)
        self.tilingsIO = TilingsIO(fPath=testDataPath)
        
        self.oparams = OParams()
        self.opHists = OParamHists()
        self.specTrajs = SpeciesTrajectories()
        self.tilings = Tilings()
    
    def loadData(self, full=False):
        self.bfTrajsIO.rff(container=self.specTrajs, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        self.tilingsIO.rff(container=self.tilings, full=full)
        
        tilings = [self.tilings[1], self.tilings[2]]
        
        Transforms(src=self.specTrajs, dst=self.opHists, oparams=self.oparams, tilings=tilings)
    
    def test_dims_from_transform(self):
        '''
        test dims field
        '''
        self.loadData()
        
        dims = np.array(self.opHists[5].dims)
        intendedDims = np.array((101,101))
        self.assertTrue(np.allclose(dims, intendedDims), msg='%s is not allclose to %s' % (dims, intendedDims))
    
    def test_edges_from_transfrom(self):
        '''
        test edges
        '''
        self.loadData()
        
        edges = self.opHists[5].getEdges()
        intendedEdges = [np.arange(100), np.arange(100)]
        self.assertTrue(np.allclose(edges, np.array(intendedEdges)), msg='%s is not allclose to %s' % (edges, intendedEdges))
    
    def test_hDims_from_transfrom(self):
        '''
        test that the dimensions of the histogram match what's given back by the dims field
        will fail if the .h field is not using numpy storage
        '''
        self.loadData()
        
        hDims = np.array(self.opHists[5].h.shape)
        intendedDims = np.array((101,101))
        self.assertTrue(np.allclose(hDims, intendedDims), msg='%s is not allclose to %s' % (hDims, intendedDims))
    
    def test_order_parameter_values_from_transfrom(self):
        '''
        test calculation of order parameter values via the reduction of a ReplicateTrajectory
        the "ground truth" comparison histogram is massive, and so is stored in a separate module
        '''
        self.loadData(full=True)
        
        orderParameterValuesArr = np.array(self.opHists[5].order_parameter_values)
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValues5Arr))

    def test_rank(self):
        '''
        test rank field
        '''
        self.loadData()
        
        rank = self.opHists[5].rank
        intendedRank = 2
        self.assertEqual(rank, intendedRank)
    