import h5py
from .replicateTrajectory import ReplicateTrajectory

class ReplicateTrajectories(object):
    def __init__(self, fPath):
        self.fPath = fPath
        self.trajectoryMap = {}

    def __getitem__(self, key):
        return self.trajectoryMap[key]

    def hasHDF5(self):
        with h5py.File(self.fPath,'r') as simF:
            if 'Simulations' in simF.keys():
                return TUREAD 
        
    def rffHDF5(self):
    # rff (read from file)
        with h5py.File(self.fPath,'r') as simF:
            for key,val in simF['Simulations'].items():
                self.trajectoryMap[int(key) - 1] = ReplicateTrajectory(trajectoryID=(int(key) - 1), hdf5TrajectoryGroup=val)