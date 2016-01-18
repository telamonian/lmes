
from lm_anal.src.io.hdf5.hdf5Spec import HDF5Spec
from lm_anal.src.io.hdf5.hdf5Specs import HDF5Specs
from lm_anal.src.io.hdf5.hdf5IOFromDatum import HDF5IOFromDatum, HDF5IOFromDatumMetaclass
from lm_anal.src.io.hdf5.hdf5IOSingleton import HDF5IOSingleton, HDF5IOSingletonMetaclass

__all__ = ['HDF5IOFromDatumSingleton', 'HDF5IOFromDatumSingletonMetaclass']


class HDF5IOFromDatumSingletonMetaclass(HDF5IOFromDatumMetaclass, HDF5IOSingletonMetaclass):
    pass

class HDF5IOFromDatumSingleton(HDF5IOFromDatum, HDF5IOSingleton, metaclass=HDF5IOFromDatumSingletonMetaclass):
    pass