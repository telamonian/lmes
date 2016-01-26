import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf'))
from lm.io.FFluxOutput_pb2 import FFluxOutput as FFluxOutputBuf
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs
from lm_anal.src.datumABC import FFluxABC

__all__ = ['FFluxOutput']

class FFluxOutput(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='int', name='number_species', paths=('number_species',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='int', name='number_tiles', paths=('number_tiles',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='int', name='tiling_id', paths=('tiling_id',), storageType='protobuf', type='scalar'),
                            DPSpec(name='basin_outputs', paths=('basin_outputs',), storageType='protobuf', type='embedded'),
                            DPSpec(name='final_output', paths=('final_output',), storageType='protobuf', type='embedded'),
                            DPSpec(name='trajectory_outputs', paths=('trajectory_outputs',), storageType='protobuf', type='embedded'))

    def __init__(self, o=None):
        super().__init__(o=o)
        self.protobuf = FFluxOutputBuf()
        
FFluxABC.register(FFluxOutput)