import numpy as np
import os,sys
from pathlib import Path
import time

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
testDataPath = thisScriptDir / Path('../../testData/biphasic_switch.lm')

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

class OParamHistsSumCheck(object):
    def checkOParamHistsSum(self, intendedSum):
        self.opHists.genSum()
        sum = np.sum(self.opHists['sum'].order_parameter_values)
        sumArr = self.opHists['sum'].order_parameter_values
        altIntendedSum = sum([np.sum(self.opHists[i].order_parameter_values) for i in range(1,11)])
        altIntendedSumArr = sum([self.opHists[i].order_parameter_values for i in range(1,11)])
         
        # if everything==0, then none of these tests are very interesting
        self.assertTrue(sum > 0)
        self.assertEqual(sum, intendedSum)
        self.assertEqual(sum, altIntendedSum)
        try:
            testBool = np.allclose(sumArr, altIntendedSumArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (sumArr.tolist(), altIntendedSumArr.tolist()))

class OParamHistsEagerTestBase(object):    
    def loadData(self, full=False):
        self.bfTrajsIO = BruteForceTrajectoriesIO(fPath=testDataPath)
        self.oparamsIO = OParamsIO(fPath=testDataPath)
        self.tilingsIO = TilingsIO(fPath=testDataPath)
        
        self.bfTrajsIO.rff(container=self.specTrajs, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        self.tilingsIO.rff(container=self.tilings, full=full)
         
        Transforms(srcs=self.specTrajs, dsts=self.opHists, oparams=self.oparams, tilings=self.tilings, tilingIDs=(1,2))
    
    def cleanUpInt(self):
        # make sure all of the .lmint/.mod stuff is cleaned up
        try:
            os.remove(str(testDataPath.with_suffix('.mod')))
        except FileNotFoundError:
            pass
        try:
            os.remove(str(testDataPath.with_suffix('.lmint')))
        except FileNotFoundError:
            pass

class OParamHistsLazyTestBase(object):
#     def setUp(self):
#         self.startTime = time.time()

#     def tearDown(self):
#         t = time.time() - self.startTime
#         print("%s: %.3f" % (self.id(), t))
    
    def loadData(self, full=False):
        self.oparams = OParams(fPath=str(testDataPath))
        self.specTrajs = SpeciesTrajectories(fPath=str(testDataPath))
        self.tilings = Tilings(fPath=str(testDataPath))
        
        transformKwargs = {'oparams':self.oparams, 'tilings':self.tilings, 'tilingIDs':(1,2)}
        
        self.opHists = OParamHists(dataToTransform=self.specTrajs, fPath=str(testDataPath), transformKwargs=transformKwargs)
    
    def loadDataEagerly(self, full=False):
        self.loadData(full=full)
        self.opHists.map
    
    def cleanUpInt(self):
        # make sure all of the .lmint/.mod stuff is cleaned up
        try:
            os.remove(str(testDataPath.with_suffix('.mod')))
        except FileNotFoundError:
            pass
        try:
            os.remove(str(testDataPath.with_suffix('.lmint')))
        except FileNotFoundError:
            pass
    
class OParamHistsFieldsTestBase(object):
    def test_dims(self):
        '''
        test dims field
        '''
        self.loadData()
         
        dims = np.array(self.opHists[5].h_dims)
        intendedDims = np.array((101,101))
        self.assertTrue(np.allclose(dims, intendedDims), msg='%s is not allclose to %s' % (dims, intendedDims))
     
    def test_edges(self):
        '''
        test edges
        '''
        self.loadData()
         
        edges = self.opHists[5].getEdges()
        intendedEdges = [np.arange(100), np.arange(100)]
        self.assertTrue(np.allclose(edges, np.array(intendedEdges)), msg='%s is not allclose to %s' % (edges, intendedEdges))
     
    def test_hDims(self):
        '''
        test that the dimensions of the histogram match what's given back by the dims field
        will fail if the .h field is not using numpy storage
        '''
        self.loadData()
         
        hDims = np.array(self.opHists[5].h.shape)
        intendedDims = np.array((101,101))
        self.assertTrue(np.allclose(hDims, intendedDims), msg='%s is not allclose to %s' % (hDims, intendedDims))
    
    def test_order_parameter_values(self):
        '''
        test calculation of order parameter values via the reduction of a ReplicateTrajectory
        the "ground truth" comparison histogram is massive, and so is stored in a separate module
        '''
#         loadTime = time.time()
        self.loadData(full=True)
#         print('load time %.3f' % (time.time() - loadTime))
        
#         arrayifyTime = time.time()
        orderParameterValuesArr = np.array(self.opHists[5].order_parameter_values)
#         print('arrayify time %.3f' % (time.time() - arrayifyTime))
        
#         checkTime = time.time()
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValues5Arr),
                msg='%s is not allclose to %s' % (orderParameterValuesArr.tolist(), intendedOrderParameterValues5Arr.tolist()))
        # alternative form for checking equality of arrays
        # self.assertTrue(not (orderParameterValuesArr - intendedOrderParameterValues5Arr).any())
#         print('check time %.3f' % (time.time() - checkTime))
    
    def test_rank(self):
        '''
        test rank field
        '''
        self.loadData()
         
        rank = self.opHists[5].rank
        intendedRank = 2
        self.assertEqual(rank, intendedRank)

class OParamHistsMethodsTestBase(object):
    def test_klDivergence(self):
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
    
    def test_order_parameter_values_addition(self):
        '''
        test the += operator
        '''
        self.loadData(full=True)
         
        opHist5ID = id(self.opHists[5])
        self.opHists[5]+=self.opHists[4]
        newOPHist5ID = id(self.opHists[5])
        orderParameterValuesArr = np.array(self.opHists[5].order_parameter_values)
        
#         checkTime = time.time()
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValues4Plus5Arr),
                        msg='%s is not allclose to %s' % (orderParameterValuesArr.tolist(), intendedOrderParameterValues4Plus5Arr.tolist()))
#         print('check time %.3f' % (time.time() - checkTime))

        self.assertEqual(opHist5ID, newOPHist5ID)
    
    def test_order_parameter_values_subtraction(self):
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
    
    def test_remask(self):
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

    def test_rethreshold(self):
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
        
    def test_reweight(self):
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