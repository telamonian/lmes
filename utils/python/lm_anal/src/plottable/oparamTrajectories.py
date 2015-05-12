import os

from ..io.fileHDF5 import FileHDF5
from .oparamTrajectory import OParamTrajectory

class OParamTrajectories(FileHDF5):
    '''
    class that represents a time-resolved trajectory calculated from an order parameter (effectively, some generic function f(species_count, degree_advancement))
    '''    
    hdf5RootPath = '/Trajectory/OParam'
    subType = OParamTrajectory
    
    def __init__(self, fPath, sim):
        self.sim = sim
        super().__init__(fPath)
    
    def _InitPlottable(self, id, **kwargs):
        self.map[id] = self.__class__.subType(id=id, sim=self.sim, **kwargs)
     
    def InitPlottable(self, id, keys=None, useInt=True, **kwargs):
        keys = [kwargs.pop('trajID')]
        self._InitPlottable(id, **kwargs)
        if useInt:
            if not self.rffHDF5(keys=[id]):
#                 self.map[id].Init()
                self.map[id].transformDatum(keys=keys)
#                 self.wtfHDF5(keys=[id])
        else:
#             self.map[id].Init()
            self.map[id].transformDatum(keys=keys)
#             self.wtfHDF5(keys=[id])
    
    def _rffHDF5(self, full=True, keys=None):
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
            self.map[key]._rffHDF5(hdf5Group=val)
        return True
            
    def _wtfHDF5(self, keys=None):
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
            self[key]._wtfHDF5(hdf5Group=hdf5Group)
            