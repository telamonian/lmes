from lm_anal.src.datum.fpt.fpts import FPTs
from lm_anal.src.datum.fpt.speciesFPT import SpeciesFPT
from lm_anal.src.io.hdf5.fpt import SpeciesFPTsIO

__all__ = ['SpeciesFPTs']

class SpeciesFPTs(FPTs):
    datumType = SpeciesFPT
    hdf5IOType = SpeciesFPTsIO
    sfileType = None