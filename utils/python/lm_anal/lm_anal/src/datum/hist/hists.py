from lm_anal.src.datum.data import Data
from lm_anal.src.datum.hist.hist import Hist

__all__ = ['Hists']

class Hists(Data):
    datumType = Hist
    
    def getSum(self):
        histsIter = self.valIter()
        self['sum'] = next(histsIter).combine(histsIter)