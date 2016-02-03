from lm_anal.src.datum.probability.interfaceFluxes import InterfaceFluxes
from lm_anal.src.datum.probability.ffluxInterfaceFlux import FFluxInterfaceFlux
from lm_anal.src.io.hdf5.probability import FFluxInterfaceFluxesIO

__all__ = ['FFluxInterfaceFluxes']

class FFluxInterfaceFluxes(InterfaceFluxes):
    datumType = FFluxInterfaceFlux
    hdf5IOType = FFluxInterfaceFluxesIO
    sfileType = None