import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf'))
from lm.io.FFluxOutput_pb2 import FFluxOutput as FFluxOutputBuf
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs
from lm_anal.src.datumABC import FFluxABC

__all__ = ['FFluxOutput']

class FFluxOutput(Datum):
    propertySpecs = DPSpecs(DPSpec(name='number_species', dtype='int', paths=('number_species',), storageType='protobuf', type='scalar'),
                            DPSpec(name='number_tiles', dtype='int', paths=('number_tiles',), storageType='protobuf', type='scalar'),
                            DPSpec(name='tiling_id', dtype='int', paths=('tiling_id',), storageType='protobuf', type='scalar'),
                            DPSpec(name='basins', paths=('basin_outputs',), storageType='protobuf', type='embedded'),
                            DPSpec(name='final', paths=('final_output',), storageType='protobuf', type='embedded'),
                            DPSpec(name='trajectories', paths=('trajectory_outputs',), storageType='protobuf', type='embedded'))

    def __init__(self, o=None):
        super().__init__(o=o)
        self.protobuf = FFluxOutputBuf()
        
FFluxABC.register(FFluxOutput)