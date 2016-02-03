from lm_anal.src.io.hdf5.probability.interfaceFluxesIO import InterfaceFluxesIO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class FFluxInterfaceFluxesIO(InterfaceFluxesIO):
    hdf5RootPath = 'FFluxInterfaceFluxes'