from collections import Counter
from itertools import chain
import matplotlib.pyplot as plt 
import numpy as np
import scipy.stats as st
from types import GeneratorType

from lm_anal.src.datum import Datum, DatumPropertySpec as DPSpec, DatumPropertySpecs as DPSpecs
from lm_anal.src.datum.tiling.tiling import Tiling
from lm_anal.src.datumABC import HistABC
from lm_anal.src.helper import Depth, DiagonalMask, histogramdd, timewith
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
        new = self.getCopy()
        new+=other
        return new
    
    def __sub__(self, other):
        new = self.getCopy()
        new-=other
        return new
    
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
    def coordinateFromIndex(self, iz):
        coords = []
        depth = Depth(iz)
        if depth==0:
            iz = [[iz]]
        elif depth==1:
            iz = [[i] for i in iz]

        for i,edges in zip(iz, self.getEdges()):
            coords.append(edges[i])

        if depth==0:
            return coords[0][0]
        if depth==1:
            return [coord for l in coords for coord in l]
        else:
            return coords

    def indexFromCoordinate(self, coords):
        iz = []
        depth = Depth(coords)
        if depth==0:
            coords = [[coords]]
        elif depth==1:
            coords = [[coord] for coord in coords]

        for coord,edges in zip(coords, self.getEdges()):
            iz.append(edges.searchsorted(coord))

        if depth==0:
            return iz[0][0]
        if depth==1:
            return [i for l in iz for i in l]
        else:
            return iz

    def coordinateSliceFromIndexSlice(self, izStart, izEnd):
        return self.coordinateFromIndex(izStart),self.indexFromCoordinate(izEnd)

    def indexSliceFromCoordinateSlice(self, coordsStart, coordsEnd):
        return self.indexFromCoordinate(coordsStart),self.indexFromCoordinate(coordsEnd)

    def drawIndicesFromRaw(self, nSample=1, hRawSum=None, hRawCumSum=None):
        '''
        draw from the distribution of h array indices, weighted by the h_raw bin counts 
        return a tuple-of-ntuples, where the first n-1 entries of the inner tuple correspond to a particular index, and the nth entry is the number of times the index was drawn
        '''
        if hRawSum is None:
            hRawSum = np.sum(self.h_raw)
        if hRawCumSum is None:
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

    def getDiagonalIndices(self, axes=None, offsets=None):
        '''
        get the indices along the diagonal of a multidimensional histogram
        '''
        allAxes = np.arange(len(self.h_raw.shape))
        axes = allAxes if axes is None else np.asarray(axes)
        offsets = np.zeros(axes.size) if offsets is None else np.asarray(offsets)
        offsetDict = {ax:offset for ax,offset in zip(axes, offsets)}

        diagLen = (np.array(self.h_raw.shape)[axes] - offsets).min()
        return [np.arange(diagLen) + offsetDict[ax] if ax in offsetDict else np.zeros(diagLen) for ax in allAxes]

    def getDownsampleFromRaw(self, nSample=None, frac=.1):
        '''
        get a downsampled copy of this histogram based on the counts in h_raw, with number of samples=nSamples
        '''
        if nSample is None:
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

    def getKLDivergence(self, other, absolute=False, normalize='mask', edges=None, sliceStart=None, sliceEnd=None, sliceCoordinate=True, sliceDiagonal=False, sliceInverse=True):
        '''
        get the Kullback-Leibler divergence between this hist and another.
        absolute: if true, return absolute value of KL div
        '''
        # implementation of KL div from scipy
        #return st.entropy(pk=self.h.flatten(), qk=other.h.flatten())

        # if the edges arg is set, call this same function multiple times with different sliceStart,sliceEnd vals
        if edges is not None:
            divKwargs = {'absolute':absolute, 'normalize':normalize, 'edges':None, 'sliceCoordinate':sliceCoordinate,
                         'sliceDiagonal':sliceDiagonal, 'sliceInverse':sliceInverse}
            divs = []
            for start,end in zip(edges[:-1], edges[1:]):
                divs.append(self.getKLDivergence(other, sliceStart=[start], sliceEnd=[end], **divKwargs))
            return divs

        selfHist,otherHist,effectiveShape = self.getKLDivergenceSetup(other=other, absolute=absolute, normalize=normalize,
                                                sliceStart=sliceStart, sliceEnd=sliceEnd, sliceCoordinate=sliceCoordinate,
                                                sliceDiagonal=sliceDiagonal, sliceInverse=sliceInverse)

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

    def getKLDivergenceArr(self, other, absolute=False, normalize='mask', sliceStart=None, sliceEnd=None, sliceCoordinate=True, sliceDiagonal=False, sliceInverse=True):
        selfHist,otherHist,effectiveShape = self.getKLDivergenceSetup(other=other, absolute=absolute, normalize=normalize,
                                                                      sliceStart=sliceStart, sliceEnd=sliceEnd, sliceCoordinate=sliceCoordinate,
                                                                      sliceDiagonal=sliceDiagonal, sliceInverse=sliceInverse)

        # calculation stuff
        retVal = self.getCopy()
        retVal.initH()
        retVal.clearVals()

        it = np.nditer((selfHist.h[effectiveShape], otherHist.h[effectiveShape]), flags=['multi_index'])
        while not it.finished:
            if it[0]==0 or it[1]==0:
                it.iternext()
                continue
            retVal.h_raw[it.multi_index] = it[0]*np.log(it[0]/it[1])
            it.iternext()

        if absolute:
            retVal.h_raw = np.abs(retVal.h_raw)
        retVal.h_cache_dirty = True
        return retVal

    def getKLDivergenceSetup(self, other, absolute, normalize, sliceStart, sliceEnd, sliceCoordinate, sliceDiagonal, sliceInverse):
        effectiveShape = ()
        for selfDim,otherDim in zip(self.h.shape, other.h.shape):
            effectiveShape+=(np.s_[:np.min((selfDim, otherDim))],)

        newHists = [None, None]
        for i,oldHist in enumerate([self,other]):
            # if we're normalizing or masking, make copies of the hists we're handed
            if normalize or (sliceStart is not None and sliceEnd is not None):
                newHists[i] = oldHist.getCopy()
            else:
                newHists[i] = oldHist

            if sliceStart is not None and sliceEnd is not None:
                newHists[i].maskSlice(start=sliceStart, end=sliceEnd, coordinate=sliceCoordinate,
                                      diagonal=sliceDiagonal, inverse=sliceInverse, normalize=False)

        if normalize=='mask':
            eitherZeroMask = np.logical_or(newHists[0].h[effectiveShape]==0, newHists[1].h[effectiveShape]==0)
        for i,oldHist in enumerate([self,other]):
            if normalize=='mask':
                # normalize the distributions in a way that takes into account the fact that we're masking out any bins that aren't nonzero in both distributions
                # zeroMask = np.ones(newHists[i].h.shape, dtype=bool)
                # zeroMask[effectiveShape] = eitherZeroMask
                newHists[i].remask(eitherZeroMask)
                newHists[i].normalize()
            elif normalize:
                # normalize both distributions in a totally straight-forward way
                newHists[i].normalize()

        selfHist,otherHist = newHists
        return selfHist,otherHist,effectiveShape

    def getStdErrArr(self, other, normalize=True):
        retVal = self.getCopy()
        retVal.initH()
        retVal.clearVals()

        if normalize:
            retVal.normalize()
            o = other.getCopy()
            o.normalize()
        else:
            o = other

        retVal.h_raw = (retVal.h - o.h)/retVal.h
        retVal.h_cache_dirty = True
        return retVal

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

    def runWithCopy(self, function, *args, **kwargs):
        '''
        helper function that deals with histogram initialization when running functions on copies of self
        '''
        retVal = self.getCopy()
        retVal.initH()

        function(retVal, *args, **kwargs)
        self.h_cache_dirty = True
        return retVal

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

    def splitCumSum(self, n=2):
        '''
        good for animating brute force histograms
        '''
        splits = np.array(self.split(n=n), dtype='O')
        for i in range(splits.size):
            for j in range(i+1, splits.size):
                splits.ravel()[i]+=splits.ravel()[j]
        return splits

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
            if not len(others) > 0:
                return self
            self.combineInPlace(*others, autothreshold=autothreshold, otherMask=otherMask)
            return self

        retVal = self.getCopy()
        retVal.initH()

        if not len(others) > 0:
            return retVal
        if isinstance(others[0], GeneratorType):
            histChain = chain([self], *others)
        else:
            histChain = chain([self], others)
        for hist in histChain:
            retVal.h_raw+=hist.h

        retVal.h_cache_dirty = True
        return retVal

    def combineInPlace(self, *others, autothreshold=False, otherMask=None):
        '''
        method to additively combine many histograms in place relative to self
        self.h.shape must == other.h.shape, but they can be otherwise dissimilar (different total N, different normalization, etc.)
        ''' 
        for other in others:
            self.h_raw+=other.h
        self.h_cache_dirty = True

    def copyCounts(self, other):
        self.h = other.h
        self.h_raw = other.h_raw
        self.h_mask = other.h_mask
        self.h_threshold = other.h_threshold
        self.h_weight = other.h_weight

    def maskDiagonalSlice(self, start, end, axes=None, coordinate=True, inverse=False, normalize=False):
        if coordinate:
            start, end = self.indexSliceFromCoordinateSlice(start, end)

        self.remask(DiagonalMask(arr=self.h_raw, sliceStart=start, sliceEnd=end, axes=axes, inverse=inverse))
        self.normalize(method=normalize)
        return self

    def maskSlice(self, start, end, axes=None, clear=True, coordinate=True, diagonal=False, inverse=False, normalize=False):
        if diagonal:
            return self.maskDiagonalSlice(start=start, end=end, axes=axes, coordinate=coordinate, inverse=inverse, normalize=normalize)
        if clear:
            self.h_mask.fill(True if inverse else False)
        if coordinate:
            start,end = self.indexSliceFromCoordinateSlice(start, end)

        slize = [slice(s, e) for s,e in zip(start, end)]
        self.h_mask[slize] = False if inverse else True
        self.normalize(method=normalize)
        self.h_cache_dirty = True
        return self

    def normalize(self, method=True):
        if method:
            if method=='raw':
                # this way makes .h_raw.sum()==1.0
                self.reweight(float(1)/np.sum(self.h_raw))
            else:
                # this way makes .h.sum()==1.0
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
            raise ValueError('In Hist.remask, mask.shape and self.h_dims should agree. mask.shape: %s, self.h_dims: %s' % (mask.shape, self.h_dims))
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

    def _sliceDiagonal(self, retVal, sliceStart, sliceEnd, axes=None, normalize=False):
        if axes is not None and not len(sliceStart)==len(sliceEnd)==len(axes):
            raise ValueError('in hist._sliceDiagonal, if axes is specific then it is required that \
                              len(sliceStart)==len(sliceEnd)==len(axes). \
                              sliceStart: %s, sliceEnd: %s, axes: %s' % (sliceStart, sliceEnd, axes))
        elif axes is None and not len(sliceStart)==len(sliceEnd)==self.rank:
            raise ValueError('in hist._sliceDiagonal, if axes is not specific then it is required that \
                              len(sliceStart)==len(sliceEnd)==self.rank. \
                              sliceStart: %s, sliceEnd: %s, self.rank: %d' % (sliceStart, sliceEnd, self.rank))

        axes = np.arange(len(self.h_raw.shape)) if axes is None else np.asarray(axes)
        slize,reducedSlize = [0]*len(self.h_raw.shape),[0]*len(self.h_raw.shape)
        for i,ax in enumerate(axes):
            slize[ax] = slice(sliceStart[i], sliceEnd[i])
            reducedSlize[ax] = slice(sliceStart[i] + 1, sliceEnd[i])
        # retVal.h_raw should start off full of 0
        retVal.h_raw[slize] = 1
        retVal.h_raw[reducedSlize] = 0
        offsetsArr = np.column_stack(retVal.h_raw.nonzero())
        offsetsArr-=offsetsArr.min(axis=1).reshape(-1,1)
        for offsets in offsetsArr:
            dI = self.getDiagonalIndices(axes=axes, offsets=offsets)
            retVal.h_raw[dI] = self.h_raw[dI]

        if normalize:
            retVal.normalize()
        return retVal

    def sliceDiagonal(self, sliceStart, sliceEnd, axes=None, inplace=False, normalize=False):
        '''
        Returns a version of the hist with only the diagonals that pass through
        the box with lower and upper corners defined by the points sliceStart and sliceEnd.
        '''
        retVal = self.runWithCopy(self._sliceDiagonal, **{'sliceStart':sliceStart, 'sliceEnd':sliceEnd,
                                                          'axes':axes, 'normalize':normalize})
        if inplace:
            # this isn't optimized
            self.copyCounts(retVal)
            return self
        else:
            return retVal

    def setObservations(self, obs):
        '''
        same as AddObservations, but clears the previously added observations (if any) first
        '''
        self.clearVals()
        self.addObservations(obs)

    def unmask(self):
        self.h_mask.fill(False)
        self.h_cache_dirty = True

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