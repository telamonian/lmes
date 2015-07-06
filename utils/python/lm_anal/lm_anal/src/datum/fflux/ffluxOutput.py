from lm_anal.src.datum import Datum
from lm_anal.src.datum import DatumPropertySpec as DPSpec
from lm_anal.src.datum import DatumPropertySpecs as DPSpecs

class FFluxOutput(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='int', name='number_species', paths=('number_species',), storageType='protoBuf', type='scalar'),
                            DPSpec(dtype='int', name='number_tiles', paths=('number_tiles',), storageType='protoBuf', type='scalar'),
                            DPSpec(dtype='int', name='tiling_id', paths=('tiling_id',), storageType='protoBuf', type='scalar'),
#                             DPSpec(name='basin_outputs', paths=('basin_outputs',), storageType='protoBuf', type='embedded'),
#                             DPSpec(name='final_output', paths=('final_output',), storageType='protoBuf', type='embedded'),
                            DPSpec(name='trajectory_outputs', paths=('trajectory_outputs',), storageType='protoBuf', type='embedded'))

