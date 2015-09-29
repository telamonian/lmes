import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../test/testData/biphasic_switch_fflux_simulations.lm')

from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import SpeciesTrajectoriesIO
 
from lm_anal.src.datum.fflux import FFluxOutputs
from lm_anal.src.datum.hist import FFluxHists
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories

from lm_anal.test.datum.hist import FFluxHistsLazyTestBase

from lm_anal.src.transform import Transforms

class FFluxOutputToOParamHistTEagerExample(object):
    def __init__(self, testDataPath=testDataPath):
        self.testDataPath = testDataPath

    def setUp(self):
        self.bfTrajIO = SpeciesTrajectoriesIO(fPath=self.testDataPath)
        self.ffluxOutsIO = FFluxOutputsIO(fPath=self.testDataPath)
        self.oparamsIO = OParamsIO(fPath=self.testDataPath)
        self.simParamsIO = SimulationParametersIO(fPath=self.testDataPath)
        self.tilingsIO = TilingsIO(fPath=self.testDataPath)
        
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
    
    def test_order_parameter_values(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.setUp()
        self.loadData(full=True)
        
        self.opVArr = np.array(self.ffluxHists[19].order_parameter_values)

class FFluxOutputToOParamHistTLazyExample(FFluxHistsLazyTestBase):
    def __init__(self, testDataPath=testDataPath):
        self.testDataPath = testDataPath
        self.loadDataEagerly()
    
    def loadData(self, full=False):
        self.ffluxOuts = FFluxOutputs(fPath=str(self.testDataPath))
        self.oparams = OParams(fPath=str(self.testDataPath))
        self.simParams = SimulationParameters(fPath=str(self.testDataPath))
        self.specTrajs = SpeciesTrajectories(fPath=str(self.testDataPath))
        self.tilings = Tilings(fPath=str(self.testDataPath))
        
        transformKwargs = {'oparams':self.oparams, 'simulationParameters':self.simParams, 'tilings':self.tilings, 'tilingIDs':(1,2)}
        
        self.ffluxHists = FFluxHists(dataToTransform={self.specTrajs, self.ffluxOuts}, fPath=str(self.testDataPath), transformKwargs=transformKwargs)
    
if __name__=='__main__':
    exam = FFluxOutputToOParamHistTEagerExample()
    exam.test_order_parameter_values()
    print(exam.opVArr.tolist())
    print(exam.opVArr.sum())
