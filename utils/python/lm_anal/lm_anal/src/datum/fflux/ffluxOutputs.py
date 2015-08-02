from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxOutput

__all__ = ['FFluxOutputs']

class FFluxOutputs(Data):
    datumType = FFluxOutput
    
    def __init__(self):
        super().__init__()