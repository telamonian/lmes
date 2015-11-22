from collections import OrderedDict
import numpy as np

# from lm.io.FFluxOutput_pb2 import TrajectoryOutput as TrajectoryOutputBuf
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs

__all__ = ['FFluxTrajectory']

class FFluxTrajectory(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='int', name='direction', paths=('direction',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='int', name='lifecycle', paths=('lifecycle',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='count', paths=('count',), storageType='numpy', type='array'),
                            DPSpec(dtype='int', name='edge_id', paths=('edge_id',), storageType='numpy', type='array'),
                            DPSpec(dtype='int', name='species_count', paths=('species_count',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='time', paths=('time',), storageType='numpy', type='array'),
                            DPSpec(dtype='int', name='trajectory_id', paths=('trajectory_id',), storageType='numpy', type='array'))

    def __init__(self, subBuf, full=False, **kwargs):
        super().__init__(full=full, **kwargs)
        if subBuf!=None:
            self.protobuf = subBuf
#         else:
#             self.protobuf = TrajectoryOutputBuf()

    def genTrajectoryPhaseMap(self, directionID):
        tpmDtype = list(zip(('trajectory_id', 'basin_id', 'phase_id'), ('int','int','int')))
        self.trajectory_phase_map = np.zeros((len(self.trajectory_id),), dtype=tpmDtype)
        self.trajectory_phase_map['trajectory_id'][:] = self.trajectory_id
        self.trajectory_phase_map['basin_id'][:] = directionID
        self.trajectory_phase_map['phase_id'][:] = self.edge_id
#         for i,trajID in enumerate(self.trajectory_id):
#             self.trajectory_phase_map[trajID] = (directionID, self.edge_id[i])
        self.tPMap = self.trajectory_phase_map