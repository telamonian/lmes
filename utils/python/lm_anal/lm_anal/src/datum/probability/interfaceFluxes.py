from lm_anal.src.datum.probability.probabilities import Probabilities
from lm_anal.src.datum.probability.interfaceFlux import InterfaceFlux
from lm_anal.src.io.hdf5.probability import InterfaceFluxesIO

__all__ = ['InterfaceFluxes']

class InterfaceFluxes(Probabilities):
    datumType = InterfaceFlux
    hdf5IOType = InterfaceFluxesIO
    sfileType = None