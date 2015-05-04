import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf'))

import numpy as np
from TrajectoryState_pb2 import TrajectoryState as TrajectoryStateBuf

class ReplicateTrajectory(object):
    def __init__(self, trajectoryID, hdf5TrajectoryGroup=None, full=False):
        self.trajectoryStateBuf = TrajectoryStateBuf()
        self.trajectoryStateBuf.trajectory_id = trajectoryID
        self.trajectoryStateBuf.cme_state.species_counts.trajectory_id = trajectoryID
        if hdf5TrajectoryGroup!=None:
            self.InitFromHdf5(hdf5TrajectoryGroup, full=full)
        self.oparamProbabilityHistMap = {}

        # pass through attributes to the underlying TrajectoryStateBuf
        self.number_entries = self.trajectoryStateBuf.cme_state.species_counts.number_entries
        self.number_species = self.trajectoryStateBuf.cme_state.species_counts.number_species
        self.trajectory_id = self.trajectoryStateBuf.cme_state.species_counts.trajectory_id
        
    def InitFromHdf5(self, hdf5TrajectoryGroup, full=False, useNPArr=True):
        '''
        initialize the data storage container underlying this ReplicateTrajectory instance, which is in turn a TrajectoryStateBuf instance (with some numpy arrays thrown in for good measure if useNPArr=True)
        '''
        self.trajectoryStateBuf.cme_state.species_counts.number_entries = hdf5TrajectoryGroup['SpeciesCounts'].shape[0]
        self.trajectoryStateBuf.cme_state.species_counts.number_species = hdf5TrajectoryGroup['SpeciesCounts'].shape[1]
        
        if full:
            if useNPArr:
                self.InitNPArrFromHdf5(hdf5TrajectoryGroup)
            else:
                self.InitBufArrFromHdf5(hdf5TrajectoryGroup)
    
    def InitNPArrFromHdf5(self, hdf5TrajectoryGroup):
        '''
        initializes species_count and time fields with numpy array based storage
        '''
        self.species_count = np.zeros(hdf5TrajectoryGroup['SpeciesCounts'].shape)
        hdf5TrajectoryGroup['SpeciesCounts'].read_direct(self.species_count)
        self.time = np.zeros(hdf5TrajectoryGroup['SpeciesCountTimes'].shape)
        hdf5TrajectoryGroup['SpeciesCountTimes'].read_direct(self.time)
    
    def InitBufArrFromHdf5(self, hdf5TrajectoryGroup):
        '''
        initializes species_count and time fields with protobuf array based storage
        all of the protobuf stuff is currently 100% python native, so probably worse for very large datasets such as these
        '''
        for val in hdf5TrajectoryGroup['SpeciesCounts']:
            self.trajectoryStateBuf.cme_state.species_counts.species_count.extend(val.tolist())
        for val in hdf5TrajectoryGroup['SpeciesCountTimes']:
            self.trajectoryStateBuf.cme_state.species_counts.time.append(val)
            
        self.species_count = self.trajectoryStateBuf.cme_state.species_counts.species_count
        self.time = self.trajectoryStateBuf.cme_state.species_counts.time
        