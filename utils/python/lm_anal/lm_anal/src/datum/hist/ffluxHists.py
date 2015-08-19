from lm_anal.src.datum.hist.ffluxHist import FFluxHist
from lm_anal.src.datum.hist.oparamHists import OParamHists
from lm_anal.src.io.hdf5.hist import FFluxHistsIO

__all__ = ['FFluxHists']

# def getFFluxHist():
#     import lm_anal.src.datum.hist.ffluxHist
#     return lm_anal.src.datum.hist.ffluxHist.FFluxHist

class FFluxHists(OParamHists):
    datumType = FFluxHist
    Hdf5IOType = FFluxHistsIO
    SFileType = None