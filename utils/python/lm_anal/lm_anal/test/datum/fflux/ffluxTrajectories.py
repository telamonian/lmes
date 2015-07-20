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
        try:
            testBool = np.allclose(countArr, intendedCount19Arr)
        except ValueError:
            testBool = False
            
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (countArr.tolist(), intendedCount19Arr.tolist()))
    
    def test_edge_id_from_hdf5_supercontainer(self):
        '''
        test the edge_id field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        edgeIDArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].edge_id)
        try:
            testBool = np.allclose(edgeIDArr, intendedEdgeID19Arr)
        except ValueError:
            testBool = False
            
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (edgeIDArr.tolist(), intendedEdgeID19Arr.tolist()))
        
    def test_species_count_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        speciesCountArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/INITIAL'].species_count)
        try:
            testBool = np.allclose(speciesCountArr, intendedSpeciesCount19Arr)
        except ValueError:
            testBool = False
            
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (speciesCountArr.tolist(), intendedSpeciesCount19Arr.tolist()))

    def test_time_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        timeArr = np.array(self.ffluxOuts[19].trajectories['FORWARD/INITIAL'].time)
        try:
            testBool = np.allclose(timeArr, intendedTime19Arr)
        except ValueError:
            testBool = False
            
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (timeArr.tolist(), intendedTime19Arr.tolist()))

    def test_trajectory_id_from_hdf5_supercontainer(self):
        '''
        test the species_count field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        trajectoryIDArr = np.array(self.ffluxOuts[19].trajectories['FORWARD/FINAL'].trajectory_id)
        try:
            testBool = np.allclose(trajectoryIDArr, intendedTrajectoryID19Arr)
        except ValueError:
            testBool = False
            
        self.assertTrue(testBool, msg='not allclose: %s\n%s' % (trajectoryIDArr.tolist(), intendedTrajectoryID19Arr.tolist()))
    
    def test_trajectoryPhaseMap_from_hdf5_supercontainer(self):
        '''
        test the generated trajectoryPhaseMap field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        phase = self.ffluxOuts[19].trajectories['FORWARD/INITIAL'].tPMap[200]
        intendendPhase = 5
        self.assertEqual(phase, intendendPhase)
        
        phase = self.ffluxOuts[19].trajectories['BACKWARD/INITIAL'].tPMap[700]
        intendendPhase = 7
        self.assertEqual(phase, intendendPhase)
