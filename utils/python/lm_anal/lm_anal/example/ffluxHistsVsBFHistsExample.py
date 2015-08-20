from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
bfRootPath = (thisScriptDir / Path('../../../../../regression/genetic_toggle_switch_test_data')).resolve()
ffluxFilePath = (thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()

class FFluxHistsVsBFHistsExample(object):
    def __init__(self, inputFilePath=inputFilePath, bfRootPath=bfRootPath, ffluxFilePath=ffluxFilePath):
        self.inputFilePath = inputFilePath
        self.inputSim = Sim(fPath=self.inputFilePath)
        
        self.bfRootPath = bfRootPath
        self.bfSims = Sims(rootPath=self.bfRootPath)
        
        self.ffluxFilePath = ffluxFilePath
        self.ffluxSim = Sim(fPath=self.ffluxFilePath)
        self.ffluxSim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':(1,2)}
    
        self.bfHist = self.bfSims[(('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), 'biphasic_switch')].oparamHists['Sum']
        self.bfHist.remask(np.zeros(self.bfHist.h_mask.shape, dtype=bool))
        self.bfHist.resliceH(np.s_[:101,:101])
        
        self.ffluxHist = self.ffluxSim.ffluxHists[19]