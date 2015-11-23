from collections import Counter
from itertools import chain
import matplotlib.pyplot as plt 
import numpy as np
import scipy.stats as st

from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs
from lm_anal.src.datum.tiling.tiling import Tiling
from lm_anal.src.datumABC import HistABC
from lm_anal.src.helper import histogramdd, timewith
from lm_anal.src.plottable import DensePlottable

__all__ = ['Hist']

class Hist(Datum, DensePlottable):
# class attributes
    propertySpecs = DPSpecs(#DPSpec(name='dims', dtype='float', storageType='numpy', type='array'),
                            #DPSpec(name='edges', dtype='float', storageType='numpy', type='array'),
                            DPSpec(name='h', dtype='float', storageType='numpy', type='histogram'))
                            # DPSpec(name='tilings', paths=('tilings',), SubDataType=Tiling, type='subData')

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
        try:
            return self.h_dims.size
        except AttributeError:
            return len(self.h_dims)

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
    def drawIndicesFromRaw(self, nSample=1, hRawSum=None, hRawCumSum=None):
        '''
        draw from the distribution of h array indices, weighted by the h_raw bin counts 
        return a tuple-of-ntuples, where the first n-1 entries of the inner tuple correspond to a particular index, and the nth entry is the number of times the index was drawn
        '''
        if hRawSum==None:
            hRawSum = np.sum(self.h_raw)
        if hRawCumSum==None:
            hRawCumSum = np.cumsum(self.h_raw)  #.reshape(self.h_raw.shape)
        
        rands = np.random.rand(nSample)*hRawSum  #).tolist()
        raveledSamples = np.searchsorted(hRawCumSum, rands)
        uniqueSamples,uniqueCounts = np.unique(raveledSamples, return_counts=True) 
        #np.column_stack(np.unravel_index(np.unique(raveledSamples, return_counts=True)[0], self.h_raw.shape) + (np.unique(raveledSamples, return_counts=True)[1],))
        return np.column_stack(np.unravel_index(uniqueSamples, self.h_raw.shape) + (uniqueCounts,))

    def getBinCenters(self):
        centers = []
        for edgeArr in self.getEdgeArrays():
            edgeDiff = (edgeArr[1:] - edgeArr[:-1])/2
            centers.append(np.concatenate(((edgeArr[0] - edgeDiff[0],), edgeArr[:-1] + edgeDiff, (edgeArr[-1] + edgeDiff[-1],))))
        return centers

    def getDownsampleFromRaw(self, nSample=None, frac=.1):
        '''
        get a downsampled copy of this histogram based on the counts in h_raw, with number of samples=nSamples
        '''
        if nSample==None:
            nSample = int(frac*np.sum(self.h_raw))
        
        downHist = self.getCopy()
        downHist.clearVals()
        
        samples = self.drawIndicesFromRaw(nSample)
        downHist.addObservationsByIndex(samples)
        return downHist

    def getEdges(self):
        '''
        rolls the 1D self.edges array into an nD array based on what's in self.dims
        '''
        return [self.h_edges[int(np.sum(self.rDims[:i])):int(np.sum(self.rDims[:i + 1]))] for i in range(self.rank)]

    def getEdgeArrays(self):
        return [np.array(edgeList) for edgeList in self.getEdges()]

    def getEdgeIndices(self):
        '''
        based on what's in self.dims, generates a list of tuples of indices that can be used to transform the 1D protobuf array in which self.edges is stored into a list of lists, one list for every dim
        '''
        return [(int(np.sum(self.rDims[:i])), int(np.sum(self.rDims[:i + 1]))) for i in range(self.rank)]

    def getEdgesWithPadding(self, paddingWidth=1):
        eWP = []
        for edges in self.getEdges():
            paddedEdges = np.zeros((edges.size + 2,))
            paddedEdges[1:-1] = edges
            paddedEdges[0] = edges[0] - paddingWidth
            paddedEdges[-1] = edges[-1] + paddingWidth
            eWP.append(paddedEdges)
        return eWP
    
    def getKLDivergence(self, other, normalize=True, absolute=False):
        '''
        get the Kullback-Leibler divergence between this hist and another.
        absolute: if true, return absolute value of KL div
        '''
        # implementation of KL div from scipy
        #return st.entropy(pk=self.h.flatten(), qk=other.h.flatten())
        
        effectiveShape = ()
        for selfDim,otherDim in zip(self.h.shape, other.h.shape):
            effectiveShape+=(np.s_[:np.min((selfDim, otherDim))],)
        
        #normalization stuff
        if normalize=='mask':
            eitherZeroMask = np.logical_or(self.h[effectiveShape]==0, other.h[effectiveShape]==0)
        
        newHists = [None, None]
        for i,oldHist in enumerate([self,other]):
            if normalize=='mask':
                # normalize the distributions in a way that takes into account the fact that we're masking out any bins that aren't nonzero in both distributions
                newHists[i] = oldHist.getCopy()
                zeroMask = np.ones(newHists[i].h.shape, dtype=bool)
                zeroMask[effectiveShape] = eitherZeroMask
                newHists[i].remask(zeroMask)
                newHists[i].normalize()
            elif normalize:
                # normalize both distributions in a totally straight-forward way
                newHists[i] = oldHist.getCopy()
                newHists[i].normalize()
            else:
                # just use whatever distributions we're handed
                newHists[i] = oldHist
        selfHist,otherHist = newHists
        
        # calculation stuff
        val = 0
        it = np.nditer((selfHist.h[effectiveShape], otherHist.h[effectiveShape]), flags=['multi_index'])
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
        
        if absolute:
            return np.abs(val)
        else:
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

    def split(self, n=2):
        splitHists = [self.getCopy() for i in range(n)]
        probabilities = [1/float(n)]*n
        for i,val in enumerate(self.h.ravel()):
            for j,splitVal in enumerate(np.random.multinomial(val, probabilities)):
                splitHists[j].h_raw.ravel()[i] = splitVal

        # mark the cache of all the hists as dirty
        for hist in splitHists:
            hist.h_cache_dirty = True

        return splitHists
    
# mutators
    def addObservations(self, obs):
        '''
        add observations by means of a list of tuples containing observations
        '''
        self.h_raw+=histogramdd(obs, bins=self.getEdges())[0]
        self.h_cache_dirty = True

    def addObservationsByIndex(self, obs):
        '''
        add observations by means of a list of tuples corresponding to the indices of the underlying histogram array rather than the bin edge values
        '''
#         with timewith('%d counting' % obs.shape[0]) as tw:
#             c = Counter(tuple((tuple(row) for row in obs)))
#         for i,val in c.items():
#             self.h_raw[i]+=val
        for row in obs:
            self.h_raw[tuple(row[:-1])] = row[-1]
        self.h_cache_dirty = True
        
    def addWeightedObservations(self, obs, weight=1.0):
        self.h_raw+=(histogramdd(obs, bins=self.getEdges())[0])*weight
        self.h_cache_dirty = True
    
    def clearVals(self):
        self.h_raw[:] = 0
        self.h_cache_dirty = True
    
    def combine(self, *others, autothreshold=False, inPlace=False, otherMask=None):
        '''
        method to additively combine many histograms
        self.h.shape must == other.h.shape, but they can be otherwise dissimilar (different total N, different normalization, etc.)
        '''
        if inPlace:
            self.combineInPlace(*others, autothreshold=autothreshold, otherMask=otherMask)
            return self
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
    
    def scaleWeight(self, weightScale):
        self.reweight(weight=self.h_weight*weightScale)
        return self
    
    def setObservations(self, obs):
        '''
        same as AddObservations, but clears the previously added observations (if any) first
        '''
        self.clearVals()
        self.addObservations(obs)

# # plotting stuff
    @property
    def axLabels(self):
        return self._axLabels
    
    @property
    def plotData(self):
        return self.h
    
#     def plot(self, fig=None, ax=None, scale='log'):
#         if fig is None:
#             fig = plt.figure(figsize=(12,12))
#         if ax is None:
#             ax = fig.gca()
#         
#         if len(self.h_dims)==1:
#             ax.plot(self.getEdgesWithPadding()[0][:-1], self.h)
#             if scale=='log':
#                 ax.set_yscale('log')
#             ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])
#         
#         elif len(self.h_dims)==2:
#             X, Y = np.meshgrid(*self.getEdgesWithPadding())
#             ax.pcolormesh(X, Y, self.h)
#             ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
#             ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])
        

HistABC.register(Hist)