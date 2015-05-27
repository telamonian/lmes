import h5py

class Data(object):
    datumType = None
    hdf5RootPath = None
    
    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath
        self.protobuf = None
        self.map = {}
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]

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
        test if an hdf5 file has a non-empty group containing data relevant to this particular Data object 
        '''
        return self.wrapperHDF5(self._has)
    
    def _rff(self, full, keys):
        if keys==None:
            keys = self.file[self.hdf5RootPath].keys()
        
        for key in keys:
            val = self.file[os.path.join(self.hdf5RootPath, key)]
            self.map[int(key)] = self.datumType(hdf5Group=val)
    
    def rff(self, full=False, keys=None):
        '''
        rff (read from file) for hdf5 files
        '''
        self.wrapperHDF5(self._rff, full=full, keys=keys)
    
    def sff(self, keys=None):
        '''
        sff (stream from file) for hdf5 files
        '''
        if keys==None:
            with h5py.File(self.fPath,'r') as lmF:
                keys = list(lmF[self.hdf5RootPath].keys())
                
        for key in keys:
            self.rff(full=True, keys=[key])
            yield self[int(key)]
            del self[int(key)]
    
#     def sff(self):
#         '''
#         sff (stream from file) for hdf5 files
#         '''
#         with h5py.File(self.fPath,'r') as lmF:
#             keys = list(lmF[self.hdf5RootPath].keys())
#         for key in keys:
#             self.rff(full=True, keys=[key])
#             yield self[int(key)]
#             del self[int(key)]

    def wrapperHDF5(self, func, **kwargs):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the hdf5 file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant hdf5 file)), just run the function (with *args)
        '''
        if self.file==None:
            with h5py.File(self.fPath,'r') as self.file:
                retVal = func(**kwargs)
            self.file = None
        else:
            retVal = func(**kwargs)
        return retVal