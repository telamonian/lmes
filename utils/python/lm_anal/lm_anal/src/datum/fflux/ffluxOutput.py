from lm_anal.python_protobuf.lm.io.FFluxOutput_pb2 import FFluxOutput as FFluxOutputBuf

from lm_anal.src.datum import Datum
from lm_anal.src.datum import DatumPropertySpec as DPSpec
from lm_anal.src.datum import DatumPropertySpecs as DPSpecs

class FFluxOutput(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='int', name='number_species', paths=('number_species',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='int', name='number_tiles', paths=('number_tiles',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='int', name='tiling_id', paths=('tiling_id',), storageType='protobuf', type='scalar'),
#                             DPSpec(name='basin_outputs', paths=('basin_outputs',), storageType='protobuf', type='embedded'),
#                             DPSpec(name='final_output', paths=('final_output',), storageType='protobuf', type='embedded'),
                            DPSpec(name='trajectory_outputs', paths=('trajectory_outputs',), storageType='protobuf', type='embedded'))

    def __init__(self, full=False):
        super().__init__(full=full)
        self.protobuf = FFluxOutputBuf()