import os

from ..datum.data import Data
from .replicateTrajectory import ReplicateTrajectory

class ReplicateTrajectories(Data):
    datumType = ReplicateTrajectory
    hdf5RootPath = 'Simulations'
    
    def __init__(self, fPath):
        super().__init__(fPath)

    def _rffHDF5(self, full, keys):
        '''
        rff (read from file)
        '''
        if keys==None:
            keys = list(self.file[self.hdf5RootPath].keys())
        
        for key in keys:
            val = self.file[os.path.join(self.hdf5RootPath, key)]
            self.map[int(key)] = ReplicateTrajectory(trajectoryID=(int(key)), hdf5TrajectoryGroup=val, full=full)
            
#     def _sffHDF5(self):
#         for key in self.file[self.hdf5RootPath].keys():
#             val = self.file[os.path.join(self.hdf5RootPath, key)]
#             self.map[int(key)] = ReplicateTrajectory(trajectoryID=(int(key)), hdf5TrajectoryGroup=val)
#         