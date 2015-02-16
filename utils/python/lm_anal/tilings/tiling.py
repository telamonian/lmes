import os,sys
sys.path.append('../../../../build/src/python/lm/io')
from Tilings_pb2 import Tilings as TilingsBuf

class Tiling(object):
    def __init__(self, tilingBuf, hdf5Tiling=None):
        if hdf5Tiling!=None:
            self.tilingBuf = tilingBuf
            self.tilingBuf.arrangement = 0
            self.tilingBuf.type = hdf5Tiling.attrs['Type']
            self.tilingBuf.id = hdf5Tiling.attrs['ID']
            self.tilingBuf.order_parameter_id = hdf5Tiling.attrs['OrderParameterID']
            self.tilingBuf.type = hdf5Tiling.attrs['Type']
            for val in  hdf5Tiling['Edges']:
                self.tilingBuf.edges.append(val)
            