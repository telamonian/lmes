import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.io.hdf5.fflux import FFluxTrajectoriesIO
from lm_anal.src.datum.fflux import FFluxOutputs
from lm_anal.src.datum.fflux import FFluxTrajectories

class FFluxTrajectoriesTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff_Output(container=self.ffluxOuts, full=full)
        self.ffluxOutsIO.rff_Trajectory(container=self.ffluxOuts, full=full)
    
    def test_get_trajectories_by_phase_from_hdf5(self):
        '''
        test the number_species field in FFluxOutput
        '''
        self.loadData(full=True)
        
        orderParameterValuesArr = np.array(self.ffluxOuts[0].getTrajectoriesByPhase(2).order_parameter_values)
        intendedOPVArr = 7
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOPVArr), 
                        msg='%s is not allclose to %s' % (orderParameterValuesArr, intendedOPVArr))
