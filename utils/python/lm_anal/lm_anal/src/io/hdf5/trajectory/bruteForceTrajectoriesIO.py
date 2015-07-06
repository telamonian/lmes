import os

from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class BruteForceTrajectoriesIO(HDF5IO):
    hdf5RootPath = 'Simulations'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='number_entries', subKey='SpeciesCounts', type='special'),
                          HDF5Spec(fullOnly=False, name='number_species', subKey='SpeciesCounts', type='special'),
                          HDF5Spec(fullOnly=False, name='trajectory_id', subKey='trajectory_id', type='special'),
                          HDF5Spec(fullOnly=True, name='species_count', subKey='SpeciesCounts', type='dataset'),
                          HDF5Spec(fullOnly=True, name='time', subKey='SpeciesCountTimes', type='dataset'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
            
    def inputNumberEntries(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name=hdf5Spec.name, val=self.file[hdf5Path][hdf5Spec.subKey].shape[0])

    def inputNumberSpecies(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name='number_species', val=self.file[hdf5Path][hdf5Spec.subKey].shape[1])

    def inputTrajectoryId(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name='trajectory_id', val=int(os.path.split(hdf5Path)[-1]))

#     def __init__(self, hdf5TrajectoryGroup=None, full=False, useNPArr=True):
#         self.full= full
#         self.trajectoryStateBuf = TrajectoryStateBuf()
#         self.useNPArr = useNPArr
#         
#         if hdf5TrajectoryGroup!=None:
#             self.InitFromHdf5(hdf5TrajectoryGroup)

#         # pass through attributes to the underlying TrajectoryStateBuf
#         self.number_entries = self.trajectoryStateBuf.cme_state.species_counts.number_entries
#         self.number_species = self.trajectoryStateBuf.cme_state.species_counts.number_species
#         self.trajectory_id = self.trajectoryStateBuf.cme_state.species_counts.trajectory_id
        
#     def Input(self, hdf5TrajectoryGroup, subCon):
#         '''
#         initialize the data storage container underlying this ReplicateTrajectory instance, which is in turn a TrajectoryStateBuf instance (with some numpy arrays thrown in for good measure if useNPArr=True)
#         '''
#         subCon.trajectoryID = int(os.path.split(hdf5TrajectoryGroup.name)[-1])
#         subCon.number_entries = hdf5TrajectoryGroup['SpeciesCounts'].shape[0]
#         subCon.number_species = hdf5TrajectoryGroup['SpeciesCounts'].shape[1]
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
        