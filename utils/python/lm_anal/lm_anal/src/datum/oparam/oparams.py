import os,sys

from lm_anal.src.datum import Data
from lm_anal.src.datum.oparam import OParam
from lm_anal.src.datum.oparam import OParamLinear
from lm_anal.src.helper import Tupify

# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf/lm/io'))
# sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf'))
from lm_anal.python_protobuf.lm.io.OrderParameters_pb2 import OrderParameters as OParamsBuf

class OParams(Data):
    datumType = OParam
    
    def __init__(self):
        super().__init__()
        self.protobuf = OParamsBuf()
    
    def combineByID(self, oparamIDs):
        oparamIDs = Tupify(oparamIDs)
        oparams = [self[oparamIDs[i]] for i in oparamIDs]
        return oparams[0].combine(oparams[1:])
    
    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            subcon = self.protobuf.order_parameters.add()
            self.map[key] = self.datumType(subcon=subcon, **kwargs)
            return self.map[key]

# import os,sys
# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))
# 
# from ..datum.data import Data
# from Tilings_pb2 import Tilings as TilingsBuf
# from .tiling import Tiling
# 
# class Tilings(Data):
#     hdf5RootPath = 'Tilings'
#     
#     def __init__(self, fPath):
#         super().__init__(fPath)
#         self.protobuf = TilingsBuf()
# 
#     def _rff(self, full, keys):
#         '''
#         rff (read from file)
#         '''
#         try:
#             self.protobuf.current_tiling_id = self.file[self.hdf5RootPath].attrs['currentTilingId']
#         except KeyError:
#             pass
#         for key,val in self.file[self.hdf5RootPath].items():
#             tilingBuf = self.protobuf.tilings.add()
#             self.map[val.attrs['ID']] = Tiling(tilingBuf=tilingBuf, hdf5TilingGroup=val)