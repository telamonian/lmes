from lm_anal.src.datum.fpt.ffluxFPT import FFluxFPT
from lm_anal.src.datum.fpt.oparamFPTs import OParamFPTs
from lm_anal.src.io.hdf5.fpt import FFluxFPTIO

__all__ = ['FFluxFPTs']

class FFluxFPTs(OParamFPTs):
    datumType = FFluxFPT
    hdf5IOType = FFluxFPTIO
    sfileType = None