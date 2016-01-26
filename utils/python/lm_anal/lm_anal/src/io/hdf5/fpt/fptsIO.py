from lm_anal.src.io.hdf5.pcloud import PCloudsIO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class FPTsIO(PCloudsIO):
    hdf5RootPath = 'FPTs'