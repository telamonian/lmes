import os,sys
sys.path.append('../../../../build/src/python/lm/io')

import h5py
from Tilings_pb2 import Tilings as TilingsBuf
from tiling import Tiling

class Tilings(object):
    def __init__(self, fPath):
        self.fPath = fPath
        self.tilingsBuf = TilingsBuf()
        self.tilingMap = {}
        
    def rffHDF5(self):
    # rff (read from file)
        with h5py.File(self.fPath,'r') as simF:
            try:
                self.tilingsBuf.current_tiling_id = simF['Tilings'].attrs['currentTilingId']
            except KeyError:
                pass
            for key,val in simF['Tilings'].items():
                tilingBuf = self.tilingsBuf.tilings.add()
                self.tilingMap[val.attrs['ID']] = Tiling(tilingBuf=tilingBuf, hdf5Tiling=val)