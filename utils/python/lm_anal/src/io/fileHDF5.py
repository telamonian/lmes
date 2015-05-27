import h5py

from .file import File

class FileHDF5(File):
    hdf5RootPath = None
    
    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath
        self.map = {}
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]

    def __iter__(self):
        return self.map.items().__iter__()

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
    
    def _rff(self, full, keys, **kwargs):
        pass
    
    def rff(self, full=False, keys=None, **kwargs):
        '''
        rff (read from file) for hdf5 files
        '''
        return self.wrapperHDF5(self._rff, full=full, keys=keys, **kwargs)
    
    def sff(self, keys=None):
        '''
        sff (stream from file) for hdf5 files
        '''
        if keys==None:
            with h5py.File(self.fPath,'r') as lmF:
                keys = list(lmF[self.hdf5RootPath].keys())
                
        for key in keys:
            try:
                self.rff(full=True, keys=[key])
            except KeyError:
                key = '%07d' % key
                self.rff(full=True, keys=[key])
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