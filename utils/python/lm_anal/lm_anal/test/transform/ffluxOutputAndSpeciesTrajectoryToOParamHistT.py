import numpy as np
import os,sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch_fflux_simulations.lm')

from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO

from lm_anal.src.datum.fflux import FFluxOutputs
from lm_anal.src.datum.hist import FFluxHists
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories

from lm_anal.src.transform import Transforms

from lm_anal.test.datum.hist.oparamHists import OParamHistsSumCheck

class FFluxOutputAndSpeciesTrajectoryToOParamHistTTestCase(unittest.TestCase, OParamHistsSumCheck):
    def setUp(self):
        self.bfTrajIO = BruteForceTrajectoriesIO(fPath=testDataPath)
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.oparamsIO = OParamsIO(fPath=testDataPath)
        self.simParamsIO = SimulationParametersIO(fPath=testDataPath)
        self.tilingsIO = TilingsIO(fPath=testDataPath)
        
        self.ffluxHists = FFluxHists()
        self.ffluxOuts = FFluxOutputs()
        self.oparams = OParams()
        self.simParams = SimulationParameters()
        self.specTraj = SpeciesTrajectories()
        self.tilings = Tilings()
        
    def loadData(self, full=False):
        self.bfTrajIO.rff(container=self.specTraj, full=full)
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        self.simParamsIO.rff(container=self.simParams, full=full)
        self.tilingsIO.rff(container=self.tilings, full=full)
        
        tilingIDs = [1,2]
        
        Transforms(srcs={self.specTraj, self.ffluxOuts}, dsts=self.ffluxHists, oparams=self.oparams, simulationParameters=self.simParams, tilings=self.tilings, tilingIDs=tilingIDs)
    
#     def test_order_parameter_values(self):
#         '''
#         test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
#         '''
#         self.loadData(full=True)
#         
#         opVArr = np.array(self.opHists[19].order_parameter_values)
#         intendedOPVArr = np.zeros(opVArr.shape)
#         try:
#             testBool = np.allclose(opVArr, intendedOPVArr)
#         except ValueError:
#             testBool = False
#         self.assertTrue(testBool, msg='not allclose: %s\n%s' % (opVArr.tolist(), intendedOPVArr.tolist()))
    
    def test_phase_weights(self):
        '''
        test the phase_weights in the histogram that results from a FFluxOutputAndSpeciesTrajectoryToOParamHistT transform
        '''
        self.loadData()
         
        pWArr = np.array(self.ffluxHists[19].phase_weights)
        intendedPWArr = np.array([(1.0, 0, 0), (4.0, 0, 1), (0.32441200324412, 0, 2), (0.08470287290969192, 0, 3), (0.01313222835809177, 0, 4), (0.0013372941301519115, 0, 5), (0.00020262032275028964, 0, 6), (0.00010893565739262883, 0, 7), (8.379665953279141e-05, 0, 8), (7.617878139344673e-05, 0, 9), (7.46850797974968e-05, 0, 10), (7.394562356187802e-05, 0, 11), (7.321348867512675e-05, 0, 12), (1.0, 1, 0), (4.0, 1, 1), (0.36330608537693004, 1, 2), (0.08568539749455897, 1, 3), (0.015897105286560104, 1, 4), (0.0030222633624638977, 1, 5), (0.001474274810957999, 1, 6), (0.0009828498739719992, 1, 7), (0.0006466117591921048, 1, 8), (0.0005825331163892836, 1, 9), (0.0005344340517332878, 1, 10), (0.0005239549526796939, 1, 11), (0.000518767279880885, 1, 12)], dtype=pWArr.dtype)
        try:
            testBool = (pWArr==intendedPWArr).all()
        except AttributeError:
            testBool = False
        self.assertTrue(testBool, msg='not equal: %s\n%s' % (pWArr.tolist(), intendedPWArr.tolist()))
        
    def test_trajectory_phase_map(self):
        '''
        test the trajectory_phase_map in the histogram that results from a FFluxOutputAndSpeciesTrajectoryToOParamHistT transform
        '''
        self.loadData(full=True)
         
        tPMArr = np.array(self.ffluxHists[19].trajectory_phase_map)
        intendedTPMArr = np.array([(1050, 0, 1), (1051, 0, 1), (1052, 0, 1), (1053, 0, 1), (1054, 0, 1), (1055, 0, 1), (1056, 0, 1), (1057, 0, 1), (1058, 0, 1), (1059, 0, 1), (1060, 0, 1), (1061, 0, 1), (1062, 0, 1), (1063, 0, 1), (1064, 0, 1), (1065, 0, 1), (1066, 0, 1), (1067, 0, 1), (1068, 0, 1), (1069, 0, 1)], dtype=tPMArr.dtype)
        try:
            testBool = (tPMArr[1050:1070]==intendedTPMArr).all()
        except AttributeError:
            testBool = False
        self.assertTrue(testBool, msg='not equal: %s\n%s' % (tPMArr[1050:1070].tolist(), intendedTPMArr.tolist()))