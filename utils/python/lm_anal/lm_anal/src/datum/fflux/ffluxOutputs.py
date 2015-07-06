from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxOutput

class FFluxOutputs(Data):
    datumType = FFluxOutput
    
    def __init__(self):
        super().__init__()