from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO

__all__ = ['FFluxOutputs']

class FFluxOutputs(Data):
    datumType = FFluxOutput
    Hdf5IOType = FFluxOutputsIO
    SFileType = None