from collections import OrderedDict
from itertools import chain
import numpy as np
import scipy.stats as sps

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sims

testRootPath='../../../../../../regression/genetic_toggle_switch_test_data'

class SimsLazyExample(object):
    def __init__(self, nDownsampled=7, testRootPath=testRootPath):
        self.nDownsampled = nDownsampled
        self.nSamples = np.logspace(2, 1+self.nDownsampled, base=10, num=self.nDownsampled, dtype=int)
        self.testRootPath = testRootPath
        self.sims = Sims(rootPath=self.testRootPath)
    
        self.opHist = self.sims[('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), ('name', 'biphasic_switch')].oparamHists['Sum']
        self.opHist.remask(np.zeros(self.opHist.h_mask.shape, dtype=bool))
        
        self.opHistDownsamples = []
        for n in self.nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(self.opHist.getDownsampleFromRaw(nSample=n))
        
#         self.klDivDict2D = OrderedDict()
#         for name,hist in zip(chain(self.nSamples, ['original']), chain(self.opHistDownsamples, [self.opHist])):
#             self.klDivDict2D[name] = self.opHist.getKLDivergence(hist, normalize='mask')
#         print('2D klDivDict contents:')
#         print(list(self.klDivDict2D.keys()))
#         print(list(self.klDivDict2D.values()))
        
        self.klDivDict1D = OrderedDict()
        opHist1D = np.array((np.arange(-100,101),[self.opHist.h.trace(offset=i) for i in np.arange(-100,101)]))
        opHist1D[1] = opHist1D[1]/opHist1D[1].sum()
        for name,hist in zip(chain(self.nSamples, ['original']), chain(self.opHistDownsamples, [self.opHist])):
            hist1D = np.array((np.arange(-100,101),[hist.h.trace(offset=i) for i in np.arange(-100,101)]))
            hist1D[1] = hist1D[1]/hist1D[1].sum()
            klDivArr = opHist1D[1]*np.log(opHist1D[1]/hist1D[1])
            klDivArr[np.isnan(klDivArr)] = 0
            klDivArr[np.isinf(klDivArr)] = 0
            self.klDivDict1D[name] = klDivArr.sum() #sps.entropy(pk=opHist1D[1]/opHist1D[1].sum(), qk=hist1D[1]/hist1D[1].sum())
        print('1D klDivDict contents:')
        print(list(self.klDivDict1D.keys()))
        print(list(self.klDivDict1D.values()))
        
if __name__=='__main__':
    SimsLazyExample()