import h5py
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
        try:
            keys = [kwargs.pop('trajID')]
        except KeyError:
            pass
        self.hdf5RootPath = os.path.join(self.__class__.hdf5RootPath, '%07d' % kwargs['oparamID'])
        self._InitPlottable(id, **kwargs)
        if useInt:
            if not self.rff(full=True, keys=[id]):
#                 self.map[id].Init()
                self.map[id].transformDatum(keys=keys)
                self.wtf(keys=[id])
        else:
#             self.map[id].Init()
            self.map[id].transformDatum(keys=keys)
            self.wtf(keys=[id])
    
    def _rff(self, full=True, keys=None):
        '''
        rff (read from file) method for HDF5 lmint files
        '''
        if keys==None:
            keys = self.file[self.hdf5RootPath].keys()
        
        for key in keys:
            try:
                val = self.file[os.path.join(self.hdf5RootPath, key)]
            except KeyError:
                return False
            self.map[key]._rff(full=full, hdf5Group=val)
        return True
            
    def _wtf(self, keys=None):
        '''
        wtf (write to file) method for HDF5 lmint files
        '''
        if keys==None:
            keys = self.map.keys()
        
        if self.hdf5RootPath in self.file:
            hdf5RootGroup = self.file[self.hdf5RootPath]
        else:    
            hdf5RootGroup = self.file.create_group(self.hdf5RootPath)
        
        for key in keys:
            trajGroupName = '%07d' % self[key].trajectory_id
            if trajGroupName in hdf5RootGroup:
                del hdf5RootGroup[trajGroupName]
            hdf5Group = hdf5RootGroup.create_group(trajGroupName)
            hdf5RootGroup[key] = h5py.SoftLink(os.path.join(self.hdf5RootPath, trajGroupName))
            self[key]._wtf(hdf5Group=hdf5Group)
            