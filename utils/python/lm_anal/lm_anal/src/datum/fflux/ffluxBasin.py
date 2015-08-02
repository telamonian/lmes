# from lm_anal.python_protobuf.lm.io.FFluxOutput_pb2 import BasinOutput as BasinOutputBuf
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs

__all__ = ['FFluxBasin']

class FFluxBasin(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='int', name='direction', paths=('direction',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='flux_out_of_tile_zero', paths=('flux_out_of_tile_zero',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='probability_i_to_i_plus_one', paths=('probability_i_to_i_plus_one',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='probability_one_to_i_plus_one', paths=('probability_one_to_i_plus_one',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='switching_rate_constant', paths=('switching_rate_constant',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='this_basin_last_visited_probability', paths=('this_basin_last_visited_probability',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='probability_i', paths=('probability_i',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='normalized_probability_i', paths=('normalized_probability_i',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='probability_i_weight', paths=('probability_i_weight',), storageType='protobuf', type='scalar'))
                            
    def __init__(self, subBuf, full=False, **kwargs):
        super().__init__(full=full, **kwargs)
        if subBuf!=None:
            self.protobuf = subBuf
#         else:
#             self.protobuf = BasinOutputBuf()