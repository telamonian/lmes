from ..datum.data import Data
from .replicateTrajectory import ReplicateTrajectory

class ReplicateTrajectories(Data):
    hdf5RootPath = 'Simulations'
    
    def __init__(self, fPath):
        super().__init__(fPath)

    def _rffHDF5(self):
        '''
        rff (read from file)
        '''
        for key,val in self.file[self.hdf5RootPath].items():
            self.map[int(key) - 1] = ReplicateTrajectory(trajectoryID=(int(key) - 1), hdf5TrajectoryGroup=val)