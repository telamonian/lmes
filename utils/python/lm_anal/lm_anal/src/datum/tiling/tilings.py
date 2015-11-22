import os,sys

from lm_anal.src.helper import Tupify
from lm_anal.src.datum.data import Data
from lm_anal.src.datum.tiling.tiling import Tiling
from lm_anal.src.datum.tiling.tilingLattice import TilingLattice
from lm_anal.src.io.hdf5.tiling import TilingsIO

from lm.io.Tilings_pb2 import Tilings as TilingsBuf

class Tilings(Data):
    datumType = Tiling
    Hdf5IOType = TilingsIO
    SFileType = None

    def __init__(self, protobuf=None, dataToTransform=None, fPath=None, transformKwargs=None):
        if protobuf==None:
            protobuf=TilingsBuf()
        super().__init__(protobuf=protobuf, dataToTransform=dataToTransform, fPath=fPath, transformKwargs=transformKwargs)

    def combine(self):
        vit = self.valIter()
        first,others = next(vit),list(vit)
        return first.combine(others)

    def combineByIDs(self, tilingIDs):
        tilings = self.sliceByKeys(tilingIDs)
        tit = tilings.valIter()
        first,others = next(tit),list(tit)
        try:
            return first.combine(others)
        except AttributeError:
            raise

    # def getByID(self, tilingIDs):
    #     tilingIDs = Tupify(tilingIDs)
    #     return [self[i] for i in tilingIDs]

    # def initDatum(self, key, **kwargs):
    #     try:
    #         return self.map[key]
    #     except KeyError:
    #         subcon = self.protobuf.tilings.add()
    #         self.map[key] = self.datumType(subcon=subcon, **kwargs)
    #         return self.map[key]



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