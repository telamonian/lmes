from itertools import chain
import numpy as np
import os
from pathlib import Path

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

DEL_FROM_MEM = False

thisScriptDir = Path(os.path.dirname(os.path.realpath(__file__)))
bfRootPath = (thisScriptDir / Path('../test/testData/biphasic_switch.lm')).resolve()
# ffluxRootPath = Path('/Users/tel/temp_data/gts_-_fflux_-_barrier_height_-_crossingsPerPhase_-_phaseZeroTime').resolve()           #(thisScriptDir / Path('../../../../../regression/biphasic_switch.lm')).resolve()

class BFHistsExample(object):
    def __init__(self, bfRootPath=bfRootPath):
        self.bfRootPath = bfRootPath
        self.bfSims = Sims(rootPath=self.bfRootPath)
        
        for key,sim in self.bfSims.items():
            print('starting probability map of %s' % str(key))
            sim.oparamHists.transformKwargs = {'tilingIDs':(3,)}   #{'tilingIDs':((1,2),3)}
#             sim.bfHists.transformKwargs = {'oparams':self.inputSim.oparams, 'tilings':self.inputSim.tilings, 'tilingIDs':((1,2),3)}]
            try:
                self.bfHist = next(self.bfSims.values().__iter__()).oparamHists[996]     #.oparamHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (3,)))]
#                 self.bfHist2D = next(self.ffluxSims.values().__iter__()).bfHists[(('InterfaceTilingID', 0), ('BinTilingIDs', (1,2)))]
                print('finished %s' % str(key))
            except: #AttributeError:
                print("%s didn't finish" % str(key))
            finally:
                if DEL_FROM_MEM:
                    del sim
                    del self.bfSims[key]
            
if __name__=='__main__':
    BFHistsExample()
