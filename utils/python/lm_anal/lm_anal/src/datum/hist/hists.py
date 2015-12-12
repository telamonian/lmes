from lm_anal.src.datum.data import Data
from lm_anal.src.datum.hist.hist import Hist

__all__ = ['Hists']

class Hists(Data):
    datumType = Hist

    def cacheSum(self):
        self['sum'] = self.genSum()

    def genSum(self):
        histsIter = self.valIter()
        return next(histsIter).combine(histsIter)