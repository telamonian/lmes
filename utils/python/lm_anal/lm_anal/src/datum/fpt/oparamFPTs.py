from lm_anal.src.datum.fpt.fpts import FPTs
from lm_anal.src.datum.fpt.oparamFPT import OParamFPT
from lm_anal.src.io.hdf5.fpt import OParamFPTIO

__all__ = ['OParamFPTs']

class OParamFPTs(FPTs):
    datumType = OParamFPT
    Hdf5IOType = OParamFPTIO
    SFileType = None