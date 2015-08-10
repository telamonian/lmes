from itertools import chain
import numpy as np
import scipy.stats as st

from lm_anal.src.helper import histogramdd
from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs
from lm_anal.src.datumABC import HistABC

__all__ = ['Hist']

class Hist(Datum):
# class attributes
    propertySpecs = DPSpecs(#DPSpec(name='dims', dtype='float', storageType='numpy', type='array'),
                            #DPSpec(name='edges', dtype='float', storageType='numpy', type='array'),
                            DPSpec(name='h', dtype='float', storageType='numpy', type='histogram'))

# operator overrides
    def __add__(self, other):
        return self.h_raw + other.h_raw
    
    def __sub__(self, other):
        return self.h_raw - other.h_raw
    
    def __iadd__(self, other):
        self.h_raw+=other.h_raw
        self.h_cache_dirty = True
        return self
        
    def __isub__(self, other):
        self.h_raw-=other.h_raw
        self.h_cache_dirty = True
        return self

# initializers
    def __init__(self, full=False):
        super().__init__(full=full)

#     def initH(self):
#         self.h_cache_dirty = True
#         self.h_threshold = 0
#         self.h_weight = 1
#         
#         self.h = np.zeros(self.dims)
#         self.h_raw = np.zeros(self.dims)
#         self.h_mask = np.zeros(self.dims, dtype=bool)

# properties
    @property
    def rDims(self):
        return np.array(self.h_dims) - 1
    
    @property
    def rank(self):
        return self.h_dims.size

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
        return [self.h_edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))] for i in range(self.rank)]
    
    def getKLDivergence(self, other, absolute=False):
        '''
        get the Kullback-Leibler divergence between this hist and another.
        absolute: if true, return absolute value of KL div
        '''
        # implementation of KL div from scipy
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
        if absolute:
            return np.abs(val)
        else:
            return val
    
    def getWeightedKLDivergence(self, other, absolute=False, weight=1):
        '''
        get the Kullback-Leibler divergence between this hist and another, where the values of the other hist are multiplied by weight.
        absolute: if true, return absolute value of KL div
        weight: unlike the standard getKLDivergence, this function does not normalize. Rather, the other distribution is weighted by this parameter
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
        get the RMSD between this hist and another.
        '''
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
        self.h_raw+=histogramdd(obs, bins=self.getEdges())[0]
        self.h_cache_dirty = True
        
    def addWeightedObservations(self, obs, weight=1.0):
        self.h_raw+=(histogramdd(obs, bins=self.getEdges())[0])*weight
        self.h_cache_dirty = True
    
    def clearVals(self):
        self.h_raw[:] = 0
        self.h_cache_dirty = True
    
    def combine(self, *others, autothreshold=False, otherMask=None):
        '''
        method to additively combine many histograms
        self.h.shape must == other.h.shape, but they can be otherwise dissimilar (different total N, different normalization, etc.)
        ''' 
        retVal = self.getCopy()
        retVal.initH()
        for other in chain([self], others):
            retVal.h_raw+=other.h
        self.h_cache_dirty = True
        return retVal

    def combineInPlace(self, *others, autothreshold=False, otherMask=None):
        '''
        method to additively combine many histograms in place relative to self
        self.h.shape must == other.h.shape, but they can be otherwise dissimilar (different total N, different normalization, etc.)
        ''' 
        for other in others:
            self.h_raw+=other.h
        self.h_cache_dirty = True
    
    def normalize(self):
        self.reweight(self.h_weight/np.sum(self.h))
    
    def recalc(self, mask=None, threshold=None, weight=None):
        '''
        one stop shop for the heavy lifting involved with manipulating the histogram data
        ensures that changing the weight won't affect the thresholding, etc.
        also ensures that all manipulations start from the same raw data, so that we're not reweighting an already reweighted histogram, etc.
        '''
        if mask is not None:
            self.h_mask = mask
        if threshold is not None:
            self.h_threshold = threshold
        if weight is not None:
            self.h_weight = weight
        
        anySet = (mask      is not None or 
                  threshold is not None or 
                  weight    is not None) 
        if anySet:
            self.h_cache_dirty = True
    
    def remask(self, mask):
        '''
        function that takes a boolean mask (same size as self.h) and zeros out the values in self.h that correspond to the 'False' values in the mask
        '''
        if mask.size!=np.product(self.h_dims):
            raise
        self.recalc(mask=mask)
        return self
    
    def rethreshold(self, threshold):
        self.recalc(threshold=threshold)
        return self
    
    def reweight(self, weight):
        self.recalc(weight=weight)
        return self
    
    def setObservations(self, obs):
        '''
        same as AddObservations, but clears the previously added observations (if any) first
        '''
        self.clearVals()
        self.addObservations(obs)
    
HistABC.register(Hist)