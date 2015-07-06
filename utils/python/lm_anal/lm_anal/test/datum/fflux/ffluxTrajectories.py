import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
from .groundTruths import intendedCount19Arr
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.datum.fflux import FFluxOutputs

class FFluxTrajectoriesTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
    
    def test_count_from_hdf5_from_supercontainer(self):
        '''
        test the count (ie order parameter count) field in FFluxTrajectory
        '''
        self.loadData(full=True)
        
        countArr = np.array(self.ffluxOuts[19].trajectories['BACKWARD/FINAL'].count)
        self.assertTrue(np.allclose(countArr, intendedCount19Arr), 
                        msg='%s is not allclose to %s' % (countArr, intendedCount19Arr))
        
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
