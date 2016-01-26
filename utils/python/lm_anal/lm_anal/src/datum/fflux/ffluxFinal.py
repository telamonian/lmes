# from lm.io.FFluxOutput_pb2 import BasinOutput as BasinOutputBuf
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs

__all__ = ['FFluxFinal']

class FFluxFinal(Datum):
    propertySpecs = DPSpecs(DPSpec(dtype='float', name='probability_i', paths=('probability_i',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='normalized_probability_i', paths=('normalized_probability_i',), storageType='numpy', type='array'),
                            DPSpec(dtype='float', name='probability_i_weight', paths=('probability_i_weight',), storageType='protobuf', type='scalar'),
                            DPSpec(dtype='float', name='switching_rate_constants', paths=('switching_rate_constants',), storageType='numpy', type='array'))
                            
    def __init__(self, subBuf, o=None, **kwargs):
        super().__init__(o=o, **kwargs)
        if subBuf!=None:
            self.protobuf = subBuf
#         else:
#             self.protobuf = BasinOutputBuf()