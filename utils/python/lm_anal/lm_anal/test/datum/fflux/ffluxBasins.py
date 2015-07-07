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
        intendedFOOTZ = 0.013150746183255506
        self.assertEqual(fOOTZ, intendedFOOTZ)
        
    def test_probability_i_to_i_plus_one_from_hdf5_supercontainer(self):
        '''
        test the probability_i_to_i_plus_one field in FFluxBasin
        '''
        self.loadData()
        
        pITIPOArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].probability_i_to_i_plus_one)
        intendedPITIPOArr = np.array([0.0, 0.08071025020177562, 0.25839793281653745, 0.17889087656529518, 0.186219739292365, 0.36363636363636365, 0.6802721088435374, 0.6711409395973155, 0.8849557522123894, 0.9523809523809523, 0.9803921568627451, 0.9900990099009901, 1.0, 0.0])
        self.assertTrue(np.allclose(pITIPOArr, intendedPITIPOArr), 
                        msg='%s is not allclose to %s' % (pITIPOArr.tolist(), intendedPITIPOArr.tolist()))
        
    def test_probability_one_to_i_plus_one_from_hdf5_supercontainer(self):
        '''
        test the probability_one_to_i_plus_one field in FFluxBasin
        '''
        self.loadData()
        
        pOTIPOArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].probability_one_to_i_plus_one)
        intendedPOTIPOArr = np.array([0.0, 0.08071025020177562, 0.020855361809244344, 0.003730833955142101, 0.000694754926469665, 0.0002526381550798782, 0.00017186269053052938, 0.00011534408760438213, 0.00010207441380918773, 9.721372743732165e-05, 9.530757591894279e-05, 9.43639365534087e-05, 9.43639365534087e-05, 0.0])
        self.assertTrue(np.allclose(pOTIPOArr, intendedPOTIPOArr), 
                        msg='%s is not allclose to %s' % (pOTIPOArr.tolist(), intendedPOTIPOArr.tolist()))
        
    def test_switching_rate_constant_from_hdf5_supercontainer(self):
        '''
        test the switching_rate_constant field in FFluxBasin
        '''
        self.loadData()
        
        sRC = self.ffluxOuts[19].basins['FORWARD'].switching_rate_constant
        intendedSRC = 1.4245720355377518e-06
        self.assertEqual(sRC, intendedSRC)
        
    def test_this_basin_last_visited_probability_from_hdf5_supercontainer(self):
        '''
        test the this_basin_last_visited_probability field in FFluxBasin
        '''
        self.loadData()
        
        tBLVP = self.ffluxOuts[19].basins['FORWARD'].this_basin_last_visited_probability
        intendedTBLVP = 0.4655573225399856
        self.assertEqual(tBLVP, intendedTBLVP)
    
    def test_normalized_probability_i_from_hdf5_supercontainer(self):
        '''
        test the normalized_probability_i field in FFluxBasin
        '''
        self.loadData()
        
        nPIArr = np.array(self.ffluxOuts[19].basins['BACKWARD'].normalized_probability_i)
        intendedNPIArr = np.array([0.0, 0.30803196355740853, 0.4722507478859476, 0.14650431612917839, 0.037852528081999584, 0.012936026072806401, 0.005943697880347311, 0.005747621441612776, 0.0023723255599249504, 0.0024823479647461486, 0.001859262926038078, 0.001796319929439493, 0.0022228425705505045, 0.0])
        self.assertTrue(np.allclose(nPIArr, intendedNPIArr), 
                        msg='%s is not allclose to %s' % (nPIArr.tolist(), intendedNPIArr.tolist()))
    
    def test_probability_i_weight_from_hdf5_supercontainer(self):
        '''
        test the probability_i_weight field in FFluxBasin
        '''
        self.loadData()
        
        pIW = self.ffluxOuts[19].basins['BACKWARD'].probability_i_weight
        intendedPIW = 0.004338316118085342
        self.assertEqual(pIW, intendedPIW)