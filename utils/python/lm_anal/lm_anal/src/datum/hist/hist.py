from abc import ABCMeta
import numpy as np

from lm_anal.src.helper import histogramdd
from lm_anal.src.datum import Datum

class HistBase(metaclass=ABCMeta):
    pass

class Hist(Datum):
    propertySpecs = {'dims':{'dtype':'float', 'storageType':'numpy', 'type':'array'},
                     'edges':{'dtype':'float', 'storageType':'numpy', 'type':'array'},
                     'h':{'dtype':'float', 'storageType':'numpy', 'type':'array'}}
    
    def __init__(self, full=False):
        super().__init__(full=full)
    
    @property
    def rDims(self):
        return np.array(self.dims) - 1
    
    @property
    def rank(self):
        return self.dims.size
    
    def initH(self):
        self.h = np.zeros(self.dims)
    
    def addObs(self, obs):
        ''' short alias for AddObservations'''
        self.addObservations(obs)
    
    def addObservations(self, obs):
        self.h+=histogramdd(obs, bins=self.getEdges())[0]
        
    def clearVals(self):
        self.h[:] = 0
    
    def getEdgeIndices(self):
        ''' based on what's in self.dims, generates a list of tuples of indices that can be used to transform the 1D protobuf array in which self.edges is stored into a list of lists, one list for every dim '''
        return [(int(np.sum(self.rDims[:i])), int(np.sum(self.rDims[:i + 1]))) for i in range(self.rank)]
    
    def getEdges(self):
        return [self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))] for i in range(self.rank)]
    
    def setObs(self, obs):
        ''' short alias for SetObservations'''
        self.setObservations(obs)
    
    def setObservations(self, obs):
        ''' same as AddObservations, but clears the previously added observations (if any) first '''
        self.clearVals()
        self.addObservations(obs)
    
HistBase.register(Hist)