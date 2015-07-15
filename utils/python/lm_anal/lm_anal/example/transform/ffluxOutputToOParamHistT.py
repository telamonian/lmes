import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../test/testData/biphasic_switch_fflux_simulations.lm')

from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO
from lm_anal.src.datum.fflux import FFluxOutputs
from lm_anal.src.datum.hist import OParamHists
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories
from lm_anal.src.transform import Transforms

class FFluxOutputToOParamHistTExample(object):
    def __init__(self, testDataPath=testDataPath):
        self.testDataPath = testDataPath

    def setUp(self):
        self.bfTrajsIO = BruteForceTrajectoriesIO(fPath=self.testDataPath)
        self.ffluxOutsIO = FFluxOutputsIO(fPath=self.testDataPath)
        self.oparamsIO = OParamsIO(fPath=self.testDataPath)
        self.simParamsIO = SimulationParametersIO(self.testDataPath)
        self.tilingsIO = TilingsIO(fPath=self.testDataPath)
        
        self.ffluxOuts = FFluxOutputs()
        self.oparams = OParams()
        self.opHists = OParamHists()
        self.simParams = SimulationParameters()
        self.specTrajs = SpeciesTrajectories()
        self.tilings = Tilings()
    
    def loadData(self, full=False):
        self.bfTrajsIO.rff(container=self.specTrajs, full=full)
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        self.simParamsIO.rff(container=self.simParams, full=full)
        self.tilingsIO.rff(container=self.tilings, full=full)
        
        tilings = [self.tilings[1], self.tilings[2]]
        
        Transforms(src=self.ffluxOuts, dst=self.opHists, oparams=self.oparams, simParams=self.simParams, specTrajs=self.specTrajs, tilings=tilings)
    
    def test_order_parameter_values(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.setUp()
        self.loadData(full=True)
        
        self.opVArr = np.array(self.opHists[19].order_parameter_values)
        
if __name__=='__main__':
    exam = FFluxOutputToOParamHistTExample()
    exam.test_order_parameter_values()
    print(exam.opVArr.tolist())
    print(exam.opVArr.sum())
