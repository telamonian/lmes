import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')

from .groundTruths import intendedOrderParameterValues5Arr, intendedOrderParameterValues4Plus5Arr
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
        
        dims = np.array(self.opHists[5].h_dims)
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
    
    def test_klDivergence_from_transform(self):
        '''
        tests the determination of the Kullbeck Liebler divergence
        for testing purposes, we have to fiddle with the data a little bit due to the general KL requirement that qk=0 implies pk=0
        '''
        self.loadData(full=True)
        
#         it = np.nditer(self.opHists[2].h, flags=['multi_index'])
#         while not it.finished:
#             if it[0]==0:
#                 self.opHists[5].h[it.multi_index[0], it.multi_index[1]] = 0
#             it.iternext()
        klDiv = self.opHists[5].compare(self.opHists[2])
        intendedKLDiv = -0.12568524690756483
        self.assertAlmostEqual(klDiv, intendedKLDiv)
    
    def test_order_parameter_values_from_transfrom(self):
        '''
        test calculation of order parameter values via the reduction of a ReplicateTrajectory
        the "ground truth" comparison histogram is massive, and so is stored in a separate module
        '''
        self.loadData(full=True)
        
        orderParameterValuesArr = np.array(self.opHists[5].order_parameter_values)
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValues5Arr), 
                        msg='%s is not allclose to %s' % (orderParameterValuesArr, intendedOrderParameterValues5Arr))
    
    def test_order_parameter_values_addition_from_transfrom(self):
        '''
        test the += operator
        '''
        self.loadData(full=True)
         
        opHist5ID = id(self.opHists[5])
        self.opHists[5]+=self.opHists[4]
        newOPHist5ID = id(self.opHists[5])
        orderParameterValuesArr = np.array(self.opHists[5].order_parameter_values)
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValues4Plus5Arr),
                        msg='%s is not allclose to %s' % (orderParameterValuesArr, intendedOrderParameterValues4Plus5Arr))
        self.assertEqual(opHist5ID, newOPHist5ID)
    
    def test_order_parameter_values_subtraction_from_transfrom(self):
        '''
        test the -= operator
        '''
        self.loadData(full=True)
         
        opHist5ID = id(self.opHists[5])
        self.opHists[5]-=self.opHists[5]
        newOPHist5ID = id(self.opHists[5])
        opvSum = np.sum(self.opHists[5].h)
        intendedOPVSum = 0.0
        self.assertEqual(opvSum, intendedOPVSum)
        self.assertEqual(opHist5ID, newOPHist5ID)
    
    def test_rank_from_transform(self):
        '''
        test rank field
        '''
        self.loadData()
        
        rank = self.opHists[5].rank
        intendedRank = 2
        self.assertEqual(rank, intendedRank)

    def test_remask_from_transform(self):
        '''
        test remask method
        we will attempt to mask first 4 nonzero values in order_parameter_values
        '''
        self.loadData(full=True)

        mask = np.zeros(self.opHists[5].order_parameter_values.shape, dtype=bool)
        opHistNonzero = self.opHists[5].order_parameter_values.nonzero()
        mask[opHistNonzero[0][:4], opHistNonzero[1][:4]] = True
        opVArr = np.array(self.opHists[5].remask(mask).order_parameter_values)
        
        intendedMaskedOPV5Arr = intendedOrderParameterValues5Arr.copy()
        intendedMaskedOPV5Arr[opHistNonzero[0][:4], opHistNonzero[1][:4]] = 0
        
        try:
            testBool = np.allclose(opVArr, intendedMaskedOPV5Arr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (opVArr.tolist(), (intendedMaskedOPV5Arr).tolist()))

    def test_rethreshold_from_transform(self):
        '''
        test rethreshold method
        '''
        self.loadData(full=True)
        
        threshold = 5
        opVArr = np.array(self.opHists[5].rethreshold(threshold).order_parameter_values)
        self.assertEqual(opVArr[np.nonzero(opVArr)].min(), threshold)
        
        threshold = 2
        opVArr = np.array(self.opHists[5].rethreshold(threshold).order_parameter_values)
        self.assertEqual(opVArr[np.nonzero(opVArr)].min(), threshold)
        
    def test_reweight_from_transform(self):
        '''
        test reweight method
        '''
        self.loadData(full=True)
        
        weight = 1.5
        oPVArr = np.array(self.opHists[5].reweight(weight).order_parameter_values)
        try:
            testBool = np.allclose(oPVArr, intendedOrderParameterValues5Arr*weight)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (oPVArr.tolist(), (intendedOrderParameterValues5Arr*weight).tolist()))
        
        weight = 7.3
        oPVArr = np.array(self.opHists[5].reweight(weight).order_parameter_values)
        try:
            testBool = np.allclose(oPVArr, intendedOrderParameterValues5Arr*weight)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (oPVArr.tolist(), (intendedOrderParameterValues5Arr*weight).tolist()))