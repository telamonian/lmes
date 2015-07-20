import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
# from .groundTruths import intendedCount19Arr, intendedEdgeID19Arr, intendedSpeciesCount19Arr, intendedTime19Arr, intendedTrajectoryID19Arr
from lm_anal.src.helper import DirectionEnum
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.datum.fflux import FFluxOutputs

class FFluxBasinsTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
    
    def test_direction_from_hdf5_supercontainer(self):
        '''
        test the direction field (and the associated enum) in FFluxBasin
        '''
        self.loadData()
        
        direction = self.ffluxOuts[19].basins['BACKWARD'].direction
        intendendDirection = DirectionEnum.Value('BACKWARD')
        self.assertEqual(direction, intendendDirection)
    
        direction = self.ffluxOuts[19].basins['FORWARD'].direction
        intendendDirection = DirectionEnum.Value('FORWARD')
        self.assertEqual(direction, intendendDirection)
    
    def test_flux_out_of_tile_zero_from_hdf5_supercontainer(self):
        '''
        test the flux_out_of_tile_zero field in FFluxBasin
        '''
        self.loadData()
        
        fOOTZ = self.ffluxOuts[19].basins['BACKWARD'].flux_out_of_tile_zero
        intendedFOOTZ = 0.023637928433073194
        self.assertEqual(fOOTZ, intendedFOOTZ)
        
    def test_probability_i_to_i_plus_one_from_hdf5_supercontainer(self):
        '''
        test the probability_i_to_i_plus_one field in FFluxBasin
        '''
        self.loadData()
        
        pITIPOArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].probability_i_to_i_plus_one)
        intendedPITIPOArr = np.array([0.0, 0.07352941176470588, 0.2631578947368421, 0.15151515151515152, 0.23255813953488372, 0.24390243902439024, 0.7692307692307693, 0.7142857142857143, 0.9090909090909091, 1.0, 1.0, 1.0, 1.0, 0.0])
        try:
            testBool = np.allclose(pITIPOArr, intendedPITIPOArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (pITIPOArr.tolist(), intendedPITIPOArr.tolist()))
        
    def test_probability_one_to_i_plus_one_from_hdf5_supercontainer(self):
        '''
        test the probability_one_to_i_plus_one field in FFluxBasin
        '''
        self.loadData()
        
        pOTIPOArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].probability_one_to_i_plus_one)
        intendedPOTIPOArr = np.array([0.0, 0.07352941176470588, 0.019349845201238388, 0.002931794727460362, 0.0006818127273163632, 0.0001662957871503325, 0.00012791983626948654, 9.137131162106182e-05, 8.306482874641983e-05, 8.306482874641983e-05, 8.306482874641983e-05, 8.306482874641983e-05, 8.306482874641983e-05, 0.0]
)
        try:
            testBool = np.allclose(pOTIPOArr, intendedPOTIPOArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (pOTIPOArr.tolist(), intendedPOTIPOArr.tolist()))
        
    def test_switching_rate_constant_from_hdf5_supercontainer(self):
        '''
        test the switching_rate_constant field in FFluxBasin
        '''
        self.loadData()
        
        sRC = self.ffluxOuts[19].basins['FORWARD'].switching_rate_constant
        intendedSRC = 1.6387447525749705e-05
        self.assertEqual(sRC, intendedSRC)
        
    def test_this_basin_last_visited_probability_from_hdf5_supercontainer(self):
        '''
        test the this_basin_last_visited_probability field in FFluxBasin
        '''
        self.loadData()
        
        tBLVP = self.ffluxOuts[19].basins['FORWARD'].this_basin_last_visited_probability
        intendedTBLVP = 0.10699624982978065
        self.assertEqual(tBLVP, intendedTBLVP)
    
    def test_normalized_probability_i_from_hdf5_supercontainer(self):
        '''
        test the normalized_probability_i field in FFluxBasin
        '''
        self.loadData()
        
        nPIArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].normalized_probability_i)
        intendedNPIArr = np.array([0.0, 0.3685161375962372, 0.4565662571463312, 0.11297381231383266, 0.036226501277728654, 0.010722065777065503, 0.003771104914008929, 0.0037157368243411425, 0.0016875366791118842, 0.000836013569533786, 0.002161008843341682, 0.0017434061735918955, 0.0010804188848755809, 0.0])
        try:
            testBool = np.allclose(nPIArr, intendedNPIArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (nPIArr.tolist(), intendedNPIArr.tolist()))
    
    def test_probability_i_weight_from_hdf5_supercontainer(self):
        '''
        test the probability_i_weight field in FFluxBasin
        '''
        self.loadData()
        
        pIW = self.ffluxOuts[19].basins['BACKWARD'].probability_i_weight
        intendedPIW = 0.011927032085228354
        self.assertEqual(pIW, intendedPIW)