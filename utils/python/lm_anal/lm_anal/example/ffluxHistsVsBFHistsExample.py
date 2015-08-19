from itertools import chain
import numpy as np

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sim, Sims

bfRootPath='../../../../../regression/genetic_toggle_switch_test_data'
ffluxFilePath='../../../../../regression/biphasic_switch.lm'

class FFluxHistsVsBFHistsExample(object):
    def __init__(self, nDownsampled=7, bfRootPath=bfRootPath, ffluxFilePath=ffluxFilePath):
        self.bfRootPath = bfRootPath
        self.bfSims = Sims(rootPath=self.bfRootPath)
        self.ffluxFilePath = ffluxFilePath
        self.ffluxSim = Sim(fPath=self.ffluxFilePath)
        self.ffluxSim.ffluxHists.transformKwargs = {'tilingIDs':(1,2)}
    
        self.bfHist = self.bfSims[(('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), 'biphasic_switch')].oparamHists['Sum']
        self.bfHist.remask(np.zeros(self.bfHist.h_mask.shape, dtype=bool))
        
        self.ffluxHist = self.ffluxSim.ffluxHists[19]
        
#         self.opHistDownsamples = []
#         for n in self.nSamples:
#             with timewith('%d' % n) as tw:
#                 self.opHistDownsamples.append(self.opHist.getDownsampleFromRaw(nSample=n))
#         
#         self.klDivDict = {}
#         for name,hist in zip(chain(self.nSamples, ['original']), chain(self.opHistDownsamples, [self.opHist])):
#             self.klDivDict[name] = self.opHist.getKLDivergence(hist, normalize='mask')
#             print(name, self.klDivDict[name])