import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

import h5py
from Tilings_pb2 import Tilings as TilingsBuf
from .tiling import Tiling

class Tilings(object):
    def __init__(self, fPath):
        self.fPath = fPath
        self.tilingsBuf = TilingsBuf()
        self.tilingMap = {}

    def __getitem__(self, key):
        return self.tilingMap[key]

    def rffHDF5(self):
    # rff (read from file)
        with h5py.File(self.fPath,'r') as simF:
            try:
                self.tilingsBuf.current_tiling_id = simF['Tilings'].attrs['currentTilingId']
            except KeyError:
                pass
            for key,val in simF['Tilings'].items():
                tilingBuf = self.tilingsBuf.tilings.add()
                self.tilingMap[val.attrs['ID']] = Tiling(tilingBuf=tilingBuf, hdf5TilingGroup=val)