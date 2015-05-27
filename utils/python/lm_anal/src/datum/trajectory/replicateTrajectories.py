import os

from src.datum.data import Data
from src.replicate.replicateTrajectory import ReplicateTrajectory

class ReplicateTrajectories(Data):
    datumType = ReplicateTrajectory
    hdf5RootPath = 'Simulations'
    
    def __init__(self, fPath):
        super().__init__(fPath)

    def _rff(self, full, keys):
        '''
        rff (read from file)
        '''
        if keys==None:
            keys = list(self.file[self.hdf5RootPath].keys())
        
        for key in keys:
            try:
                val = self.file[os.path.join(self.hdf5RootPath, str(key))]
            except KeyError:
                key = '%07d' % key
                val = self.file[os.path.join(self.hdf5RootPath, str(key))]
            self.map[int(key)] = ReplicateTrajectory(trajectoryID=(int(key)), hdf5TrajectoryGroup=val, full=full)
            
#     def _sff(self):
#         for key in self.file[self.hdf5RootPath].keys():
#             val = self.file[os.path.join(self.hdf5RootPath, key)]
#             self.map[int(key)] = ReplicateTrajectory(trajectoryID=(int(key)), hdf5TrajectoryGroup=val)
#         