from lm_anal.src.io.hdf5 import HDF5IOFromDatumSingleton
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

__all__ = ['ModelsIO']

class ModelsIO(HDF5IOFromDatumSingleton):
    hdf5RootPath = 'Model'