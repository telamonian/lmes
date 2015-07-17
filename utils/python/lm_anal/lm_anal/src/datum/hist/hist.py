from abc import ABCMeta
import numpy as np
import scipy.stats as st

from lm_anal.src.helper import histogramdd
from lm_anal.src.datum import Datum

class HistBase(metaclass=ABCMeta):
    pass

class Hist(Datum):
# class attributes
    propertySpecs = {'dims':{'dtype':'float', 'storageType':'numpy', 'type':'array'},
                     'edges':{'dtype':'float', 'storageType':'numpy', 'type':'array'},
                     'h':{'dtype':'float', 'storageType':'numpy', 'type':'array'}}

# operator overrides
    def __iadd__(self, other):
        self.h+=other.h
        return self
        
    def __isub__(self, other):
        self.h-=other.h
        return self

# initializers
    def __init__(self, full=False):
        super().__init__(full=full)

    def initH(self):
        self.h = np.zeros(self.dims)

# properties
    @property
    def rDims(self):
        return np.array(self.dims) - 1
    
    @property
    def rank(self):
        return self.dims.size

# aliases
    def addObs(self, obs):
        ''' short alias for AddObservations'''
        self.addObservations(obs)

    def compare(self, other):
        '''short alias for getKLDivergence'''
        return self.getKLDivergence(other)
    
    def setObs(self, obs):
        ''' short alias for SetObservations'''
        self.setObservations(obs)

# accessors
    def getEdgeIndices(self):
        '''
        based on what's in self.dims, generates a list of tuples of indices that can be used to transform the 1D protobuf array in which self.edges is stored into a list of lists, one list for every dim
        '''
        return [(int(np.sum(self.rDims[:i])), int(np.sum(self.rDims[:i + 1]))) for i in range(self.rank)]
    
    def getEdges(self):
        '''
        rolls the 1D self.edges array into an nD array based on what's in self.dims
        '''
        return [self.edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))] for i in range(self.rank)]
    
    def getKLDivergence(self, other):
        '''
        get the Kullback-Leibler divergence between this hist and another. alternatively, can be implemented using the entopy function from scipy.stats
        '''
        # scipy-based calculation
        #return st.entropy(pk=self.h.flatten(), qk=other.h.flatten())
        
        # currently, both distributions get normalized in a totally straight-forward way 
        sNormed = self.h/np.sum(self.h)   #[other.h!=0])
        oNormed = other.h/np.sum(other.h) #[self.h!=0])
        # alternatively, the distributions could be normalized in a way that takes into account the fact that we're masking out any bins that aren't nonzero in both distributions
        #sNormed = self.h/np.sum(self.h[other.h!=0])
        #oNormed = other.h/np.sum(other.h[self.h!=0])
        val = 0
        it = np.nditer((sNormed, oNormed), flags=['multi_index'])
        while not it.finished:
            if it[0]==0 or it[1]==0:
                it.iternext()
                continue
            val+=it[0]*np.log(it[0]/it[1])
            it.iternext()
        return val
    
    def getWeightedKLDivergence(self, other, weight=1):
        '''
        get the Kullback-Leibler divergence between this hist and another, where the values of the other hist are multiplied by weight.
        '''
        val = 0
        it = np.nditer((self.h, other.h), flags=['multi_index'])
        while not it.finished:
            if it[0]==0 or it[1]==0:
                it.iternext()
                continue
            val+=it[0]*np.log(it[0]/(it[1]*weight))
            it.iternext()
        return val
    
    def getWeightedRMSD(self, other, weight=1):
        '''
        get the RMSD between this hist and another. implemented using the entopy function from scipy.stats
        '''
        # scipy-based calculation
        # return st.entropy(pk=self.h.flatten(), qk=other.h.flatten())
        val = 0
        count = 0
        it = np.nditer((self.h, other.h), flags=['multi_index'])
        while not it.finished:
            if it[0]==0 or it[1]==0:
                it.iternext()
                continue
            val+=(it[0]-it[1]*weight)**2
            count+=1
            it.iternext()
        val = np.sqrt(val/count)
        return val
    
# mutators
    def addObservations(self, obs):
        self.h+=histogramdd(obs, bins=self.getEdges())[0]
        
    def addWeightedObservations(self, obs, weight=1.0):
        self.h+=(histogramdd(obs, bins=self.getEdges())[0])*weight
    
    def clearVals(self):
        self.h[:] = 0
    
    def reweight(self, weight):
        self.h*=weight
        return self
    
    def setObservations(self, obs):
        '''
        same as AddObservations, but clears the previously added observations (if any) first
        '''
        self.clearVals()
        self.addObservations(obs)
    
HistBase.register(Hist)