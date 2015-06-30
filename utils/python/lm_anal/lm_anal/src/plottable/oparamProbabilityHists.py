import os

from lm_anal.src.io.ioHDF5 import FileHDF5
from .oparamProbabilityHist import OParamProbabilityHist

class OParamProbabilityHists(FileHDF5):
    hdf5RootPath = '/Hist/OParam'
    subType = OParamProbabilityHist
    
    def __init__(self, fPath, sim):
        self.sim = sim
        super().__init__(fPath)
    
    def _InitPlottable(self, id, **kwargs):
        self.map[id] = self.__class__.subType(id=id, sim=self.sim, **kwargs)
     
    def InitPlottable(self, id, useInt=True, **kwargs):
        self._InitPlottable(id, **kwargs)
        if useInt:
            if not self.rff(keys=[id]):
                self.map[id].Init()
                self.map[id].transformDatum()
                self.wtf(keys=[id])
        else:
            self.map[id].Init()
            self.map[id].transformDatum()
            self.wtf(keys=[id])
    
    def _rff(self, full=True, keys=None):
        '''
        rff (read from file) method for HDF5 lmint files
        '''
        if keys==None:
            keys = self.file[self.hdf5RootPath].keys()
        
        for key in keys:
            try:
                val = self.file[os.path.join(self.__class__.hdf5RootPath, key)]
            except KeyError:
                return False
            self.map[key]._rff(hdf5Group=val)
        return True
            
    def _wtf(self, keys=None):
        '''
        wtf (write to file) method for HDF5 lmint files
        '''
        if keys==None:
            keys = self.map.keys()
        
        if self.__class__.hdf5RootPath not in self.file:
            hdf5RootGroup = self.file.create_group(self.__class__.hdf5RootPath)
        else:
            hdf5RootGroup = self.file[self.__class__.hdf5RootPath]    
        
        for key in keys:
            if key in hdf5RootGroup:
                del hdf5RootGroup[key]
            hdf5Group = hdf5RootGroup.create_group(key)
            self[key]._wtf(hdf5Group=hdf5Group)
        
#         if self.hdf5RootPath in self.file:
#             del self.file[self.hdf5RootPath]
#         hdf5RootGroup = self.file.create_group(hdf5RootPath)
#         for key,opprobhist in self:
#             hdf5Group = hdf5RootGroup.create_group(key)
#             opprobhist._wtf(hdf5Group=hdf5Group)