import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
from .groundTruths import intendedCount19Arr, intendedEdgeID19Arr, intendedSpeciesCount19Arr, intendedTime19Arr, intendedTrajectoryID19Arr
from lm_anal.src.helper import DirectionEnum, LifecycleEnum
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.datum.fflux import FFluxOutputs

class FFluxTrajectoriesTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
    
    def test_direction_from_hdf5_supercontainer(self):
        '''
        test the direction field (and the associated enum) in FFluxTrajectory
        '''
        self.loadData()
        
        direction = self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].direction
        intendendDirection = DirectionEnum.Value('BACKWARD')
        self.assertEqual(direction, intendendDirection)
    
        direction = self.ffluxOuts[19].trajectories['FORWARD/INITIAL'].direction
        intendendDirection = DirectionEnum.Value('FORWARD')
        self.assertEqual(direction, intendendDirection)
    
    def test_lifecycle_from_hdf5_supercontainer(self):
        '''
        test the lifecycle field (and the associated enum) in FFluxTrajectory
        '''
        self.loadData()
        
        lifecycle = self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].lifecycle
        intendendLifecycle = LifecycleEnum.Value('FINAL')
        self.assertEqual(lifecycle, intendendLifecycle)
    
        lifecycle = self.ffluxOuts[19].trajectories['FORWARD/INITIAL'].lifecycle
        intendendLifecycle = LifecycleEnum.Value('INITIAL')
        self.assertEqual(lifecycle, intendendLifecycle)
    
    def test_count_from_hdf5_supercontainer(self):
        '''
        test the count (ie order parameter count) field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        countArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].count)
        self.assertTrue(np.allclose(countArr, intendedCount19Arr), 
                        msg='%s is not allclose to %s' % (countArr.tolist(), intendedCount19Arr.tolist()))
    
    def test_edge_id_from_hdf5_supercontainer(self):
        '''
        test the edge_id field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        edgeIDArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].edge_id)
        self.assertTrue(np.allclose(edgeIDArr, intendedEdgeID19Arr), 
                        msg='%s is not allclose to %s' % (edgeIDArr.tolist(), intendedEdgeID19Arr.tolist()))
        
    def test_species_count_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        speciesCountArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/INITIAL'].species_count)
        self.assertTrue(np.allclose(speciesCountArr, intendedSpeciesCount19Arr), 
                        msg='%s is not allclose to %s' % (speciesCountArr.tolist(), intendedSpeciesCount19Arr.tolist()))

    def test_time_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        timeArr = np.array(self.ffluxOuts[19].trajectories['FORWARD/INITIAL'].time)
        self.assertTrue(np.allclose(timeArr, intendedTime19Arr), 
                        msg='%s is not allclose to %s' % (timeArr.tolist(), intendedTime19Arr.tolist()))

    def test_trajectory_id_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        trajectoryIDArr = np.array(self.ffluxOuts[19].trajectories['FORWARD/FINAL'].trajectory_id)
        self.assertTrue(np.allclose(trajectoryIDArr, intendedTrajectoryID19Arr), 
                        msg='%s is not allclose to %s' % (trajectoryIDArr.tolist(), intendedTrajectoryID19Arr.tolist()))
            
#     def test_get_trajectories_by_phase_from_hdf5_from_supercontainer(self):
#         '''
#         test the number_species field in FFluxOutput
#         '''
#         self.loadData(full=True)
#         
#         orderParameterValuesArr = np.array(self.ffluxOuts[0].getTrajectoriesByPhase(2).order_parameter_values)
#         intendedOPVArr = 7
#         self.assertTrue(np.allclose(orderParameterValuesArr, intendedOPVArr), 
#                         msg='%s is not allclose to %s' % (orderParameterValuesArr, intendedOPVArr))
