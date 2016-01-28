import os

from lm_anal.src.io.hdf5.hdf5IO import HDF5IO

__all__ = ['HDF5IOSingleton', 'HDF5IOSingletonMetaclass']

class HDF5IOSingletonMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        if 'dataType' in dct and dct['dataType'] is not None:
            if hasattr(dct['dataType'], 'singletonKey'):
                dct['singletonKey'] = dct['dataType'].singletonKey
        return super().__new__(cls, clsname, bases, dct)

class HDF5IOSingleton(HDF5IO, metaclass=HDF5IOSingletonMetaclass):
    def _has(self):
        return os.path.join(self.hdf5RootPath, self.singletonKey) in self.file

    def has(self):
        return self.wrapperHDF5(self._has)

    def _keys(self):
        '''
        this is for a filling a DataSingleton, so return just that one singleton key
        '''
        return [self.singletonKey] if os.path.join(self.hdf5RootPath, self.singletonKey) in self.file else []