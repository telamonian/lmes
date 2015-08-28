from itertools import chain
import numpy as np

from lm_anal.src.helper import timewith
from lm_anal.src.main import Sims

testRootPath='../../../../../../regression/genetic_toggle_switch_test_data'

class SimsLazyExample(object):
    def __init__(self, nDownsampled=7, testRootPath=testRootPath):
        self.nDownsampled = nDownsampled
        self.nSamples = np.logspace(2, 1+self.nDownsampled, base=10, num=self.nDownsampled, dtype=int)
        self.testRootPath = testRootPath
        self.sims = Sims(rootPath=self.testRootPath)
    
        self.opHist = self.sims[(('degradation', '0.25000'), ('production', '1.00000'), ('startingInBasin', 'A'), 'biphasic_switch')].oparamHists['Sum']
        self.opHist.remask(np.zeros(self.opHist.h_mask.shape, dtype=bool))
        
        self.opHistDownsamples = []
        for n in self.nSamples:
            with timewith('%d' % n) as tw:
                self.opHistDownsamples.append(self.opHist.getDownsampleFromRaw(nSample=n))
        
        self.klDivDict = {}
        for name,hist in zip(chain(self.nSamples, ['original']), chain(self.opHistDownsamples, [self.opHist])):
            self.klDivDict[name] = self.opHist.getKLDivergence(hist, normalize='mask')
            print(name, self.klDivDict[name])
            
if __name__=='__main__':
    SimsLazyExample()