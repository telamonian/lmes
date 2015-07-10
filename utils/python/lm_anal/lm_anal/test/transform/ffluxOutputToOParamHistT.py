import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../testData/biphasic_switch_fflux_simulations.lm')

from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO
from lm_anal.src.datum.fflux import FFluxOutputs
from lm_anal.src.datum.hist import OParamHists
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories
from lm_anal.src.transform import Transforms

import unittest

class FFluxOutputToOParamHistTTestCase(unittest.TestCase):
    def setUp(self):
        self.bfTrajsIO = BruteForceTrajectoriesIO(fPath=testDataPath)
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.oparamsIO = OParamsIO(fPath=testDataPath)
        self.tilingsIO = TilingsIO(fPath=testDataPath)
        
        self.ffluxOuts = FFluxOutputs()
        self.oparams = OParams()
        self.opHists = OParamHists()
        self.specTrajs = SpeciesTrajectories()
        self.tilings = Tilings()
    
    def loadData(self, full=False):
        self.bfTrajsIO.rff(container=self.specTrajs, full=full)
        self.ffluxOutsIO.rff(container=self.ffluxOuts, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        self.tilingsIO.rff(container=self.tilings, full=full)
        
        tilings = [self.tilings[1], self.tilings[2]]
        
        Transforms(src=self.ffluxOuts, dst=self.opHists, oparams=self.oparams, specTrajs=self.specTrajs, tilings=tilings)
    
    def test_order_parameter_values(self):
        '''
        test the order_parameter_values in the histogram that results from a FFluxTrajectoryToOParamHistT transform
        '''
        self.loadData(full=True)
        
        opVArr = np.array(self.opHists[19].order_parameter_values)
        print(opVArr.tolist())
        print(opVArr.sum())
        intendedOPVArr = np.zeros(opVArr.shape)
#         self.assertTrue(np.allclose(opVArr, intendedOPVArr), 
#                         msg='%s is not allclose to %s' % (opVArr.tolist(), intendedOPVArr.tolist()))
    