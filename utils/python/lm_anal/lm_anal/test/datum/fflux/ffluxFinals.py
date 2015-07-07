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
        intendedNPIArr = np.array([0.0, 0.1685286975142742, 0.2596058374478095, 0.06545679564291343, 0.01734156982149253, 0.007243820741418165, 0.006096191846746076, 0.005767933015542406, 0.007972192029605016, 0.019397293950068204, 0.07054829257815604, 0.2249624316694031, 0.14707894374257094, 0.0])
        self.assertTrue(np.allclose(nPIArr, intendedNPIArr), 
                        msg='%s is not allclose to %s' % (nPIArr.tolist(), intendedNPIArr.tolist()))
    
    def test_probability_i_weight_from_hdf5_supercontainer(self):
        '''
        test the probability_i_weight field in FFluxFinal
        '''
        self.loadData()
        
        pIW = self.ffluxOuts[19].final.probability_i_weight
        intendedPIW = 0.009137992026641959
        self.assertEqual(pIW, intendedPIW)
        
    def test_switching_rate_constants_from_hdf5_supercontainer(self):
        '''
        test the normalized_probability_i field in FFluxFinal
        '''
        self.loadData()
        
        sRCArr = np.array(self.ffluxOuts[19].final.switching_rate_constants)
        intendedSRCArr = np.array([1.4245720355377518e-06, 1.240956178466704e-06])
        self.assertTrue(np.allclose(sRCArr, intendedSRCArr), 
                        msg='%s is not allclose to %s' % (sRCArr.tolist(), intendedSRCArr.tolist()))