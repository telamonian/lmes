from lm_anal.src.datum.hist.hists import Hists
from lm_anal.src.datum.hist.oparamHist import OParamHist

__all__ = ['OParamHists']

class OParamHists(Hists):
    datumType = OParamHist
    
    def __init__(self):
        super().__init__()