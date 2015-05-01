import h5py

class Data(object):
    hdf5RootPath = None
    
    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath
        self.protobuf = None
        self.map = {}

    def __getitem__(self, key):
        return self.map[key]

    def _hasHDF5(self):
        if self.hdf5RootPath in self.file:
            if len(self.file[self.hdf5RootPath].keys()) > 0:
                return True
            else:
                return False
        else:
            return False

    def _rffHDF5(self, simF):
        pass
        
    def hasHDF5(self):
        '''
        test if an hdf5 file has a non-empty group containing data relevant to this particular Data object 
        '''
        return self.wrapperHDF5(self._hasHDF5)
#         if self.file==None:
#             with h5py.File(self.fPath,'r') as self.file:
#                 return self.hasHDF5InnerLoop()
#         else:
#             return self.hasHDF5InnerLoop()
    
    def rffHDF5(self): 
        '''
        rff (read from file) for hdf5 files
        '''
        self.wrapperHDF5(self._rffHDF5)
#         with h5py.File(self.fPath,'r') as self.file:
#             self._rffHDF5(self, self.file)

    def wrapperHDF5(self, func, *args):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the hdf5 file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant hdf5 file)), just run the function (with *args)
        '''
        if self.file==None:
            with h5py.File(self.fPath,'r') as self.file:
                retVal = func(*args)
            self.file = None
        else:
            retVal = func(*args)
        return retVal