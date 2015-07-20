import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.datum.fflux import FFluxOutputs

class FFluxFinalsTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
    
    def test_normalized_probability_i_from_hdf5_supercontainer(self):
        '''
        test the normalized_probability_i field in FFluxFinal
        '''
        self.loadData()
        
        nPIArr = np.array(self.ffluxOuts[19].final.normalized_probability_i)
        intendedNPIArr = np.array([0.0, 0.051038094495580714, 0.06264335646258878, 0.0354464871779969, 0.015252771852579224, 0.006031896682331968, 0.004989390291953673, 0.004799346322550579, 0.010185304772892766, 0.03177013864162024, 0.09424706659742561, 0.3784152048792951, 0.30518094182318445, 0.0])
        try:
            testBool = np.allclose(nPIArr, intendedNPIArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (nPIArr.tolist(), intendedNPIArr.tolist()))
    
    def test_probability_i_weight_from_hdf5_supercontainer(self):
        '''
        test the probability_i_weight field in FFluxFinal
        '''
        self.loadData()
        
        pIW = self.ffluxOuts[19].final.probability_i_weight
        intendedPIW = 0.014428621881428233
        self.assertEqual(pIW, intendedPIW)
        
    def test_switching_rate_constants_from_hdf5_supercontainer(self):
        '''
        test the normalized_probability_i field in FFluxFinal
        '''
        self.loadData()
        
        sRCArr = np.array(self.ffluxOuts[19].final.switching_rate_constants)
        intendedSRCArr = np.array([1.6387447525749705e-05, 1.963480477213353e-06])
        try:
            testBool = np.allclose(sRCArr, intendedSRCArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (sRCArr.tolist(), intendedSRCArr.tolist()))