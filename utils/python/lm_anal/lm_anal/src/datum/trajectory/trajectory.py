import os, sys

from lm.io.TrajectoryState_pb2 import TrajectoryState as TrajectoryStateBuf
from lm_anal.src.datum import Datum
from lm_anal.src.datumABC.trajectory import TrajectoryABC
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['Trajectory']

class Trajectory(Datum):
    propertySpecs = DatumSpecs(DatumSpec(name='number_entries', dtype='int', paths=('cme_state','species_counts','number_entries'), storageType='protobuf', type='scalar'),
                               DatumSpec(name='time', dtype='float', paths=('cme_state','species_counts','time'), storageType='protobuf', type='array'),
                               DatumSpec(name='trajectory_id', dtype='int', paths=('trajectory_id',), storageType='protobuf', type='scalar'))
    
    @property
    def key(self):
        return ('id', self.trajectory_id)
#     propertySpecs = {'number_entries':{'dtype':'int', 'paths':('cme_state','species_counts','number_entries'), 'storageType':'protobuf', 'type':'scalar'},
#                      'time':{'dtype':'float', 'paths':('cme_state','species_counts','time'), 'storageType':'protobuf', 'type':'array'},
#                      'trajectory_id':{'dtype':'int', 'paths':('trajectory_id',), 'storageType':'protobuf', 'type':'scalar'}}

#     @property
#     def number_entries(self):
#         return self.trajectoryStateBuf.cme_state.species_counts.number_entries
#     @number_entries.setter
#     def number_entries(self, val):
#         self.trajectoryStateBuf.cme_state.species_counts.number_entries = val
    
#     @property
#     def number_species(self):
#         return self.protobuf.cme_state.species_counts.number_species
#     @number_species.setter
#     def number_species(self, val):
#         self.protobuf.cme_state.species_counts.number_species = val
    
#     @property
#     def species_count(self):
#         return self.protobuf.cme_state.species_counts.species_count
#     @species_count.setter
#     def species_count(self, val):
#         self.protobuf.cme_state.species_counts.species_count.extend(val.flatten())
#     
#     @property
#     def time(self):
#         return self.protobuf.cme_state.species_counts.time
#     @time.setter
#     def time(self, val):
#         self.protobuf.cme_state.species_counts.time.extend(val.flatten())
    
#     @property
#     def trajectory_id(self):
#         return self.protobuf.trajectory_id
#     @trajectory_id.setter
#     def trajectory_id(self, val):
#         self.protobuf.trajectory_id = val
#     
    def __init__(self, full=False):
        super().__init__(full=full)
        self.protobuf = TrajectoryStateBuf()
        
TrajectoryABC.register(Trajectory)