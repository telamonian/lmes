import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf/lm/io'))

from bisect import bisect
from Hist_pb2 import Hist as HistBuf
import numpy as np

class Hist(object):
    def __init__(self,boundedness,dims,edges,rank):
        self.InitBuf(boundedness,dims,edges,rank)
        
    def InitBuf(self,boundedness,dims,edges,rank):
        self.histBuf = HistBuf()
        # passthroughs
        self.boundedness = self.histBuf.boundedness
        self.dims = self.histBuf.dims
        self.edges = self.histBuf.edges
        self.rank = self.histBuf.rank
        self.vals = self.histBuf.vals
        
        self.boundedness.extend(np.array(boundedness).flatten().tolist())
        self.dims.extend(np.array(dims).flatten().tolist())
        self.edges.extend(np.array(edges).flatten())
        self.rank = rank
        self.vals.extend(np.zeros(np.prod(np.array(dims))))
        
        # reduced dimensions, used in later calculations
        self.rDims = np.array(self.dims) - 1
        
    def AddData(self, data):
        for datum in data:
            self.vals[self.FindTileIndex(datum)]+=1
    
    def FindTileIndex(self, datum):
        tileIndex = 0
        for i in range(self.rank):
            tileIndex+=np.prod(self.dims[1+i:self.rank])*bisect(self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))], datum[i])
        return int(tileIndex)
    
    def FindTileIndices(self, datum):
        tileIndices = np.zeros(datum.shape)
        for i in range(self.rank):
            tileIndices[i] = bisect(self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))], datum[i])
        return tileIndices