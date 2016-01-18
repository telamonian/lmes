from lm_anal.src.io.hdf5 import HDF5IOFromDatumSingleton, HDF5Spec, HDF5Specs

__all__ = ['ModelIO']

class ModelIO(HDF5IOFromDatumSingleton):
    hdf5RootPath = 'Model'