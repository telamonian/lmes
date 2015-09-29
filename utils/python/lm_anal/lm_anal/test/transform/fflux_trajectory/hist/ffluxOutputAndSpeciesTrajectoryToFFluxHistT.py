import unittest
from lm_anal.test.datum.hist import FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase

class FFluxOutputAndSpeciesTrajectoryToFFluxHistTTestCase(unittest.TestCase, FFluxHistsLazyTestBase, FFluxHistsFieldsTestBase):
    def setUp(self):
        pass
        self.cleanUpInt()
        
    def tearDown(self):
        pass
        self.cleanUpInt()

# import numpy as np
# import os,sys
# import unittest
# 
# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch_fflux_simulations.lm')
# 
# from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
# from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
# from lm_anal.src.io.hdf5.oparam import OParamsIO
# from lm_anal.src.io.hdf5.tiling import TilingsIO
# from lm_anal.src.io.hdf5.trajectory import SpeciesTrajectoriesIO
# 
# from lm_anal.src.datum.fflux import FFluxOutputs
# from lm_anal.src.datum.hist import FFluxHists
# from lm_anal.src.datum.parameter import SimulationParameters
# from lm_anal.src.datum.oparam import OParams
# from lm_anal.src.datum.tiling import Tilings
# from lm_anal.src.datum.trajectory import SpeciesTrajectories
# 
# from lm_anal.src.transform import Transforms
# 
# from lm_anal.test.datum.hist.oparamHists import OParamHistsSumCheck
# 
# class FFluxOutputAndSpeciesTrajectoryToFFluxHistTTestCase(unittest.TestCase, OParamHistsSumCheck):
#     def setUp(self):
#         self.bfTrajIO = SpeciesTrajectoriesIO(fPath=testDataPath)
#         self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
#         self.oparamsIO = OParamsIO(fPath=testDataPath)
#         self.simParamsIO = SimulationParametersIO(fPath=testDataPath)
#         self.tilingsIO = TilingsIO(fPath=testDataPath)
#         
#         self.ffluxHists = FFluxHists()
#         self.ffluxOuts = FFluxOutputs()
#         self.oparams = OParams()
#         self.simParams = SimulationParameters()
#         self.specTraj = SpeciesTrajectories()
#         self.tilings = Tilings()
#         
#     def loadData(self, full=False):
#         self.bfTrajIO.rff(container=self.specTraj, full=full)
#         self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
#         self.oparamsIO.rff(container=self.oparams, full=full)
#         self.simParamsIO.rff(container=self.simParams, full=full)
#         self.tilingsIO.rff(container=self.tilings, full=full)
#         
#         tilingIDs = [1,2]
#         
#         Transforms(srcs={self.specTraj, self.ffluxOuts}, dsts=self.ffluxHists, oparams=self.oparams, simulationParameters=self.simParams, tilings=self.tilings, tilingIDs=tilingIDs)
#     
#     def test_order_parameter_values(self):
#         '''
#         test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
#         '''
#         self.loadData(full=True)
#             
#         opVArr = np.array(self.ffluxHists[19].order_parameter_values)
#         highAArr = np.array([[7.220809863051713e-05, 6.587461566266914e-06, 4.832841849959998e-07, 2.6400704159309915e-07, 2.416420924979999e-07],
#                               [0.0001803673385375774, 2.0984137137967825e-06, 3.894788790064249e-06, 1.1741598274927106e-08, 1.2138016997637741e-06],
#                               [0.00038718100349412345, 1.37911915413442e-05, 7.430754336846725e-08, 0.0, 1.977754813428722e-06],
#                               [0.0003521400664428631, 2.985418777382966e-06, 9.783099682669265e-07, 3.0416330769334984e-08, 9.665683699919996e-07],
#                               [0.00046331078708562224, 3.682625489669518e-05, 2.472333297717747e-07, 9.665683699919996e-07, 4.4618073444723005e-08]])
#         transitionArr = np.array([[2.404232027723169e-08, 8.386855910662219e-09, 5.591237273774812e-09, 1.5655464366569475e-08, 1.5655464366569475e-08],
#                                 [1.5655464366569475e-08, 5.591237273774812e-09, 8.386855910662219e-09, 7.2686084559072555e-09, 8.386855910662219e-09],
#                                 [1.1182474547549623e-08, 8.386855910662219e-09, 2.2364949095099247e-08, 8.386855910662219e-09, 0.0],
#                                 [1.1741598274927106e-08, 2.0128454185589325e-08, 1.1741598274927106e-08, 0.0, 0.0],
#                                 [4.920288800921835e-08, 4.081603209855613e-08, 8.386855910662219e-09, 0.0, 0.0]])
#         highBArr = np.array([[0.0038326798691807515, 0.0035616936737728274, 0.011164734128624069, 0.011861913344350798, 0.017631549397427816],
#                               [0.00035103757783221535, 0.00030680433815603154, 0.0003029660362942473, 0.000299373444932851, 0.0023356453142345983],
#                               [5.044405546069489e-06, 7.069146657402612e-07, 1.2427989745130248e-06, 6.2104591977626805e-06, 0.000293679714464961],
#                               [7.995469301497982e-09, 1.3586706575272794e-08, 0.0, 5.591237273774812e-09, 6.078043938253289e-07],
#                               [6.189868683728785e-07, 0.0, 5.966219192777793e-07, 0.0, 0.0]])
#         highAIndex = np.s_[23:28,1:6]
#         transitionIndex = np.s_[13:18,13:18]
#         highBIndex = np.s_[1:6,23:28]
#         arrDict = {'highA': (highAArr, highAIndex),
#                    'transition': (transitionArr, transitionIndex),
#                    'highB': (highBArr, highBIndex)}
#           
#         for name,(arr,i) in arrDict.items():
#             self.assertTrue(np.allclose(opVArr[i], arr), msg='%s not allclose: %s\n%s' % (name, opVArr[i].tolist(), arr.tolist()))
#           
#     def test_basin_weights(self):
#         '''
#         test the basin_weights in the histogram that results from a FFluxOutputAndSpeciesTrajectoryToOParamHistT transform
#         '''
#         self.loadData()
#              
#         bWArr = np.array(self.ffluxHists[19].basin_weights)
#         try:
#             intendedBWArr = np.array([(0.0008848915291201344, 0), (0.021040380759303427, 1)], dtype=bWArr.dtype)
#         except ValueError:
#             intendedBWArr = np.zeros(bWArr.shape)
#         try:
#             testBool = (bWArr==intendedBWArr).all()
#         except (AttributeError, UnboundLocalError) as e:
#             testBool = False
#         self.assertTrue(testBool, msg='not equal: %s\n%s' % (bWArr.tolist(), intendedBWArr.tolist()))
#            
#     def test_phase_weights(self):
#         '''
#         test the phase_weights in the histogram that results from a FFluxOutputAndSpeciesTrajectoryToOParamHistT transform
#         '''
#         self.loadData()
#              
#         pWArr = np.array(self.ffluxHists[19].phase_weights)
#         intendedPWArr = np.array([(1.0, 0, 0), (4.0, 0, 1), (0.31496062992125984, 0, 2), (0.07874015748031496, 0, 3), (0.01874765654293213, 0, 4), (0.0053564732979806086, 0, 5), (0.0038260523557004347, 0, 6), (0.002732894539786025, 0, 7), (0.0018219296931906831, 0, 8), (0.0018219296931906831, 0, 9), (0.0018219296931906831, 0, 10), (0.0018219296931906831, 0, 11), (0.0018219296931906831, 0, 12), (1.0, 1, 0), (4.0, 1, 1), (0.17857142857142858, 1, 2), (0.06868131868131869, 1, 3), (0.008176347462061749, 1, 4), (0.0004492498605528433, 1, 5), (0.00010957313672020569, 1, 6), (9.961194247291427e-05, 1, 7), (7.662457113301098e-05, 1, 8), (7.662457113301098e-05, 1, 9), (7.662457113301098e-05, 1, 10), (7.662457113301098e-05, 1, 11), (7.662457113301098e-05, 1, 12)], dtype=pWArr.dtype)
#         try:
#             testBool = (pWArr==intendedPWArr).all()
#         except AttributeError:
#             testBool = False
#         self.assertTrue(testBool, msg='not equal: %s\n%s' % (pWArr.tolist(), intendedPWArr.tolist()))
#          
#     def test_trajectory_phase_map(self):
#         '''
#         test the trajectory_phase_map in the histogram that results from a FFluxOutputAndSpeciesTrajectoryToOParamHistT transform
#         '''
#         self.loadData(full=True)
#           
#         tPMArr = np.array(self.ffluxHists[19].trajectory_phase_map)
#         
#         basinSwitchArr = np.array([(365, 0, 12), (366, 0, 12), (367, 0, 12), (368, 0, 12), (369, 0, 12), (370, 0, 12), (371, 0, 12), (372, 0, 12), (373, 0, 12), (374, 0, 12), (375, 0, 12), (376, 0, 12), (377, 1, 0), (378, 1, 0), (379, 1, 0), (380, 1, 0), (381, 1, 1), (382, 1, 1), (383, 1, 1), (384, 1, 1)], dtype=tPMArr.dtype)
#         basinSwitchIndex = np.s_[365:385]
# 
#         self.assertTrue((tPMArr[basinSwitchIndex]==basinSwitchArr).all(), 
#                          msg='not equal: %s\n%s' % (tPMArr[basinSwitchIndex].tolist(), basinSwitchArr.tolist()))