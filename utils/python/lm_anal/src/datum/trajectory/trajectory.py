import os, sys

from src.datum.datum import Datum
from src.datum.datum import DatumMetaclass

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf/lm/io'))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf'))
from TrajectoryState_pb2 import TrajectoryState as TrajectoryStateBuf

class Trajectory(Datum, metaclass=DatumMetaclass):
        # pass through attributes to the underlying TrajectoryStateBuf
    propertySpecs = {'number_entries':{'type':'scalar', 'storageType':'protoBuf', 'paths':('cme_state','species_counts','number_entries')},
                     'number_species':{'type':'scalar', 'storageType':'protoBuf', 'paths':('cme_state','species_counts','number_species')},
                     'species_count':{'type':'array', 'storageType':'protoBuf', 'paths':('cme_state','species_counts','species_count')},
                     'time':{'type':'array', 'storageType':'protoBuf', 'paths':('cme_state','species_counts','time')},
                     'trajectory_id':{'type':'scalar', 'storageType':'protoBuf', 'paths':('trajectory_id',)}}
    
#     @property
#     def number_entries(self):
#         return self.trajectoryStateBuf.cme_state.species_counts.number_entries
#     @number_entries.setter
#     def number_entries(self, val):
#         self.trajectoryStateBuf.cme_state.species_counts.number_entries = val
    
#     @property
#     def number_species(self):
#         return self.protoBuf.cme_state.species_counts.number_species
#     @number_species.setter
#     def number_species(self, val):
#         self.protoBuf.cme_state.species_counts.number_species = val
    
#     @property
#     def species_count(self):
#         return self.protoBuf.cme_state.species_counts.species_count
#     @species_count.setter
#     def species_count(self, val):
#         self.protoBuf.cme_state.species_counts.species_count.extend(val.flatten())
#     
#     @property
#     def time(self):
#         return self.protoBuf.cme_state.species_counts.time
#     @time.setter
#     def time(self, val):
#         self.protoBuf.cme_state.species_counts.time.extend(val.flatten())
    
#     @property
#     def trajectory_id(self):
#         return self.protoBuf.trajectory_id
#     @trajectory_id.setter
#     def trajectory_id(self, val):
#         self.protoBuf.trajectory_id = val
#     
    def __init__(self, full=False):
        super().__init__(full=full)
        self.protoBuf = TrajectoryStateBuf()