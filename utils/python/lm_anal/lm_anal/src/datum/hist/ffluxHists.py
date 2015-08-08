from lm_anal.src.datum.hist.ffluxHist import FFluxHist
from lm_anal.src.datum.hist.oparamHists import OParamHists

__all__ = ['FFluxHists']

# def getFFluxHist():
#     import lm_anal.src.datum.hist.ffluxHist
#     return lm_anal.src.datum.hist.ffluxHist.FFluxHist

class FFluxHists(OParamHists):
    datumType = FFluxHist
    
    def __init__(self):
        super().__init__()