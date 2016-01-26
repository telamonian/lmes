from lm_anal.src.io.hdf5.probability.probabilitiesIO import ProbabilitiesIO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class FluxesIO(ProbabilitiesIO):
    hdf5RootPath = 'Fluxes'