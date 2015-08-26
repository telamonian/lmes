from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
bfRootPath = (thisScriptDir / Path('../../../../../regression/genetic_toggle_switch_test_data')).resolve()
# ffluxRootPath = Path('/Users/tel/temp_data/gts_fflux_barrier_height_vs_crossingsPerPhase_sweep_long').resolve()
ffluxRootPath = Path('../../../../../regression/tmp_biphasic_switch.lm')

class FFluxHistsVsBFHistsExample(object):
    def __init__(self, inputFilePath=inputFilePath, bfRootPath=bfRootPath, ffluxRootPath=ffluxRootPath):
        # inputSim stuff
        self.inputFilePath = inputFilePath
        self.inputSim = Sim(fPath=self.inputFilePath)
        
        # bfSims stuff
        self.bfRootPath = bfRootPath
        self.bfSims = Sims(rootPath=self.bfRootPath)
        
        self.bfHist = self.bfSims[(('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), 'biphasic_switch')].oparamHists['Sum']
        self.bfHist.remask(np.zeros(self.bfHist.h_mask.shape, dtype=bool))
        self.bfHist.resliceH(np.s_[:101,:101])
        
        # ffluxSims stuff
        self.ffluxRootPath = ffluxRootPath
        self.ffluxSims = Sims(rootPath=self.ffluxRootPath)
        for ffluxSim in self.ffluxSims.values():
            ffluxSim.ffluxHists.transformKwargs = {'tilingIDs':((1,2),3)}
#             ffluxSim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}
        
        self.ffluxHist = next(self.ffluxSims.values().__iter__()).ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))]
#         self.ffluxHist = self.ffluxSims[(('theta1.0e+00', 'cpp1.0e+06'), 'genetic_toggle_switch')].ffluxHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (1, 2)))]
        
if __name__=='__main__':
    FFluxHistsVsBFHistsExample()
    print('hey')