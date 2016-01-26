from lm_anal.src.datum.hist.ffluxInterfaceHist import FFluxInterfaceHist
from lm_anal.src.datum.hist.oparamHists import OParamHists
from lm_anal.src.io.hdf5.hist import FFluxInterfaceHistsIO

__all__ = ['FFluxInterfaceHists']

class FFluxInterfaceHists(OParamHists):
    datumType = FFluxInterfaceHist
    hdf5IOType = FFluxInterfaceHistsIO
    sfileType = None