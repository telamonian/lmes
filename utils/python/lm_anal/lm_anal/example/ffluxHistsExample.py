from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
inputFilePath = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
ffluxRootPath = Path('/Users/tel/temp_data/gts_fflux_barrier_height_vs_crossingsPerPhase_vs_interfacesCount').resolve()           #(thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()

class FFluxHistsExample(object):
    def __init__(self, inputFilePath=inputFilePath, ffluxRootPath=ffluxRootPath):
        self.inputFilePath = inputFilePath
        self.inputSim = Sim(fPath=self.inputFilePath)
        
        self.ffluxRootPath = ffluxRootPath
        self.ffluxSims = Sims(rootPath=self.ffluxRootPath)
        
        for key,sim in self.ffluxSims.items():
            print('starting probability map of %s' % str(key))
            sim.ffluxHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),0)}
            try:
                sim.ffluxHists.map
                print('finished %s' % str(key))
            except AttributeError:
                print("%s didn't finish" % str(key))
            
if __name__=='__main__':
    FFluxHistsExample()
