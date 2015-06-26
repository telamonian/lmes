import h5py
import os,sys
import numpy as np

from src.datum.datum import DatumMetaclass
from src.datum.trajectory.trajectory import Trajectory

class SpeciesTrajectory(Trajectory, metaclass=DatumMetaclass):
    # data spec
    propertySpecs = {'species_count':{'dtype':'int', 'storageType':'numpy', 'type':'array'},
                     'time':{'dtype':'float', 'storageType':'numpy', 'type':'array'}}

#     arrays = ('species_count', 'time')
#     scalars = ('number_entries', 'number_species', 'trajectory_id')
#     
#     # pass through attributes to the underlying TrajectoryStateBuf
#     @property
#     def species_count(self):
#         return self._species_count
#     @species_count.setter
#     def species_count(self, val):
#         # initialize the array if it doesn't alreay exist
#         self.getArray(dims=val.shape, dtype=val.dtype, name='_species_count')
#         val.read_direct(self._species_count)
#      
#     @property
#     def time(self):
#         return self._time
#     @time.setter
#     def time(self, val):
#         # initialize the array if it doesn't alreay exist
#         self.getArray(dims=val.shape, dtype=val.dtype, name='_time') 
#         val.read_direct(self._time)

#         # pass through attributes to the underlying TrajectoryStateBuf
#         self.number_entries = self.trajectoryStateBuf.cme_state.species_counts.number_entries
#         self.number_species = self.trajectoryStateBuf.cme_state.species_counts.number_species
#         self.trajectory_id = self.trajectoryStateBuf.cme_state.species_counts.trajectory_id
    
#     def Input(self, someData, full=None):
#         if full!=None:
#             self.full = full
#         if isinstance(someData, h5py.Group):
#             self.InputHdf5(someData)
#         else:
#             raise
#         
#     def InputHdf5(self, hdf5TrajectoryGroup):
#         '''
#         initialize the data storage container underlying this ReplicateTrajectory instance, which is in turn a TrajectoryStateBuf instance (with some numpy arrays thrown in for good measure if useNPArr=True)
#         '''
#         self.trajectoryID = int(os.path.split(hdf5TrajectoryGroup.name)[-1])
#         self.trajectoryStateBuf.cme_state.species_counts.number_entries = hdf5TrajectoryGroup['SpeciesCounts'].shape[0]
#         self.trajectoryStateBuf.cme_state.species_counts.number_species = hdf5TrajectoryGroup['SpeciesCounts'].shape[1]
#         self.trajectoryStateBuf.cme_state.species_counts.trajectory_id = self.trajectoryID
#         
#         if self.full:
#             if self.useNPArr:
#                 self.InputNPArrFromHdf5(hdf5TrajectoryGroup)
#             else:
#                 self.InputBufArrFromHdf5(hdf5TrajectoryGroup)
#     
#     def InputNPArrFromHdf5(self, hdf5TrajectoryGroup):
#         '''
#         initializes species_count and time fields with numpy array based storage
#         '''
#         self.species_count = np.zeros(hdf5TrajectoryGroup['SpeciesCounts'].shape)
#         hdf5TrajectoryGroup['SpeciesCounts'].read_direct(self.species_count)
#         self.time = np.zeros(hdf5TrajectoryGroup['SpeciesCountTimes'].shape)
#         hdf5TrajectoryGroup['SpeciesCountTimes'].read_direct(self.time)
#     
#     def InputBufArrFromHdf5(self, hdf5TrajectoryGroup):
#         '''
#         initializes species_count and time fields with protobuf array based storage
#         all of the protobuf stuff is currently 100% python native, so probably worse for very large datasets such as these
#         '''
#         for val in hdf5TrajectoryGroup['SpeciesCounts']:
#             self.trajectoryStateBuf.cme_state.species_counts.species_count.extend(val.tolist())
#         for val in hdf5TrajectoryGroup['SpeciesCountTimes']:
#             self.trajectoryStateBuf.cme_state.species_counts.time.append(val)
#             
#         self.species_count = self.trajectoryStateBuf.cme_state.species_counts.species_count
#         self.time = self.trajectoryStateBuf.cme_state.species_counts.time
        