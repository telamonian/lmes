from bisect import bisect
import numpy as np
import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../../'))
sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf/lm/io'))

from Hist_pb2 import Hist as HistBuf
from .plottable import Plottable
from lm_anal.src.helper import histogramdd

class Hist(Plottable):
    def __init__(self,dims,edges,rank):
        self._Init(dims,edges,rank)
        
        self.InitVals()
        #self.InitVals_Buf()
    
    @property
    def rank(self):
        return self.histBuf.rank
    @rank.setter
    def rank(self, val):
        self.histBuf.rank = int(val)
    
    def _Init(self, dims, edges, rank):
        self.histBuf = HistBuf()
        # passthroughs
        self.dims = self.histBuf.dims
        self.edges = self.histBuf.edges
        
        self.edges.extend(np.array(edges).flatten())
        self.dims.extend(np.array(dims).flatten().tolist())     # tolist() avoids a nasty error where protobuf considers numpy int64 to be different from python int
        self.rank = rank
        self.rDims = np.array(self.dims) - 1                    # reduced dimensions, used in later calculations
    
    def InitVals(self):
        self.vals = np.zeros(self.dims)
    
    def AddObs(self, obs):
        ''' short alias for AddObservations'''
        self.AddObservations(obs)
    
    def AddObservations(self, obs):
        self.vals+=histogramdd(obs, bins=self.GetEdges())[0]
    
    def ClearVals(self):
        self.vals[:] = 0
    
    def GetEdgeIndices(self):
        ''' based on what's in self.dims, generates a list of tuples of indices that can be used to transform the 1D protobuf array in which self.edges is stored into a list of lists, one list for every dim '''
        return [(int(np.sum(self.rDims[:i])), int(np.sum(self.rDims[:i + 1]))) for i in range(self.rank)]
    
    def GetEdges(self):
        return [self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))] for i in range(self.rank)]
    
    def SetObs(self, obs):
        ''' short alias for SetObservations'''
        self.SetObservations(obs)
    
    def SetObservations(self, obs):
        ''' same as AddObservations, but clears the previously added observations (if any) first '''
        self.ClearVals()
        self.AddObservations(obs)
    
    # functions with _Buf suffix can be used for a 100% protobuf based implementation
    def InitVals_Buf(self):
        self.vals = self.histBuf.vals
        self.vals.extend(np.zeros(np.prod(np.array(self.dims))))
    
    def AddObs_Buf(self, obs):
        ''' short alias for AddObservations_Buf'''
        self.AddObservations_Buf(obs)
    
    def AddObservations_Buf(self, obs):
        for datum in obs:
            self.vals[self.FindTileIndex_Buf(datum)]+=1
    
    def ClearVals_Buf(self):
        del self.vals[:]
        self.vals.extend(np.zeros(np.prod(np.array(self.dims))))
    
    def FindTileIndex_Buf(self, datum):
        tileIndex = 0
        for i in range(self.rank):
            tileIndex+=np.prod(self.dims[1+i:self.rank])*bisect(self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))], datum[i])
        return int(tileIndex)
    
    def FindTileIndices_Buf(self, datum):
        tileIndices = np.zeros(datum.shape)
        for i in range(self.rank):
            tileIndices[i] = bisect(self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))], datum[i])
        return tileIndices
    
    def SetObs_Buf(self, obs):
        ''' short alias for SetObservations_Buf'''
        self.SetObservations_Buf(obs)
    
    def SetObservations_Buf(self, obs):
        self.ClearVals_Buf()
        self.AddObservations_Buf(obs)