from collections import namedtuple
import h5py
import os

from src.helper import CamelCaseUpper
from src.io.io import IO

# class to hold descriptions of the hdf5 file spec for specific pieces of data
HDF5Spec = namedtuple('HDF5Spec', ('fullOnly', 'name', 'subKey', 'type'))

class HDF5IO(IO):
    hdf5RootPath = None
    hdf5Specs = None
    
    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath

    def input(self, full, hdf5Path, subCon):
        for spec in (spec for spec in self.hdf5Specs if (not spec.fullOnly or full)):
            if spec.type=='array':
                self.inputArray(hdf5Path=hdf5Path, hdf5Spec=spec, subCon=subCon)
            elif spec.type=='attribute':
                self.inputAttribute(hdf5Path=hdf5Path, hdf5Spec=spec, subCon=subCon)
            elif spec.type=='special':
                self.__getattribute__('input' + CamelCaseUpper(spec.name))(hdf5Path=hdf5Path, hdf5Spec=spec, subCon=subCon)
            else:
                raise
    
    def inputArray(self, hdf5Path, hdf5Spec, subCon):
        subCon.setArray(name=hdf5Spec.name, val=self.file[hdf5Path][hdf5Spec.subKey])
    
    def inputAttribute(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name=hdf5Spec.name, val=self.file[hdf5Path].attrs[hdf5Spec.subKey])

    def _has(self):
        if self.hdf5RootPath in self.file:
            if len(self.file[self.hdf5RootPath].keys()) > 0:
                return True
            else:
                return False
        else:
            return False
        
    def has(self):
        '''
        test if an hdf5 file has a non-empty group containing data relevant to this particular object
        '''
        return self.wrapperHDF5(self._has)

    def _keys(self):
        '''
        basic hdf5 version of keys. Assumes that relevant data is located in each of the subgroups of self.hdf5RootPath
        '''
        return list(self.file[self.hdf5RootPath].keys())  

    def keys(self):
        return self.wrapperHDF5(self._keys)

    def _rff(self, container, full, keys):
        '''
        internal rff (read from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            subCon = container.initDatum(key=int(key), full=full)
            try:
                self.input(full=full, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), subCon=subCon)
            except KeyError:
                del container[int(key)]
                key = '%07d' % key
                subCon = container.initDatum(key=int(key), full=full)
                self.input(full=full, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), subCon=subCon)
    
    def rff(self, full=False, keys=None, **kwargs):
        '''
        rff (read from file) for hdf5 files
        '''
        return self.wrapperHDF5(self._rff, full=full, keys=keys, **kwargs)
    
    def sff(self, full=False, keys=None):
        '''
        sff (stream from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
                
        for key in keys:
            try:
                self.rff(full=full, keys=[key])
            except KeyError:
                key = '%07d' % key
                self.rff(full=full, keys=[key])
            yield self[int(key)]
            del self[int(key)]
        
    def _wtf(self, keys):
        pass
        
    def wtf(self, keys=None):
        '''
        wtf (write to file) for hdf5 files
        '''
        self.wrapperHDF5(self._wtf, mode='a', keys=keys)

    def wrapperHDF5(self, func, mode='r', **kwargs):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the hdf5 file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant hdf5 file)), just run the function (with *args)
        '''
        if self.file==None:
            try:
                with h5py.File(self.fPath, mode) as self.file:
                    retVal = func(**kwargs)
            except OSError:
                self.file = None
                return False
            self.file = None
        else:
            retVal = func(**kwargs)
        return retVal