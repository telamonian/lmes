from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFlux

from lm_anal.python_protobuf.lm.io.FFluxOutput_pb2 import FFluxOutput as FFluxOutputBuf

class FFluxOutputs(Data):
    datumType = FFlux
    
    def __init__(self):
        super().__init__()
        self.protobuf = FFluxOutputBuf()
