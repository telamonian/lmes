import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

from ..datum.data import Data
from Tilings_pb2 import Tilings as TilingsBuf
from .tiling import Tiling

class Tilings(Data):
    hdf5RootPath = 'Tilings'
    
    def __init__(self, fPath):
        super().__init__(fPath)
        self.protobuf = TilingsBuf()

    def _rffHDF5(self, full, keys):
        '''
        rff (read from file)
        '''
        try:
            self.protobuf.current_tiling_id = self.file[self.hdf5RootPath].attrs['currentTilingId']
        except KeyError:
            pass
        for key,val in self.file[self.hdf5RootPath].items():
            tilingBuf = self.protobuf.tilings.add()
            self.map[val.attrs['ID']] = Tiling(tilingBuf=tilingBuf, hdf5TilingGroup=val)