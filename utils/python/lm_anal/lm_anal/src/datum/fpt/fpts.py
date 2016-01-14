from lm_anal.src.datum.pcloud import PClouds
from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.io.hdf5.fpt import FPTIO

__all__ = ['FPTs']

class FPTs(PClouds):
    datumType = FPT
    Hdf5IOType = FPTIO
    SFileType = None