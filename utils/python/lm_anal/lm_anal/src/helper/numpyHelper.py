from itertools import product
import numpy as np

__all__ = ['Cartesian', 'DiagonalMask', 'DiagonalIndices', 'DiagonalSliceIndices', 'FastStack', 'FastHStack', 'FastVStack']

def Cartesian(arrays, out=None):
    """
    Generate a cartesian product of input arrays.

    Parameters
    ----------
    arrays : list of array-like
        1-D arrays to form the cartesian product of.
    out : ndarray
        Array to place the cartesian product in.

    Returns
    -------
    out : ndarray
        2-D array of shape (M, len(arrays)) containing cartesian products
        formed of input arrays.

    Examples
    --------
    >>> cartesian(([1, 2, 3], [4, 5], [6, 7]))
    array([[1, 4, 6],
           [1, 4, 7],
           [1, 5, 6],
           [1, 5, 7],
           [2, 4, 6],
           [2, 4, 7],
           [2, 5, 6],
           [2, 5, 7],
           [3, 4, 6],
           [3, 4, 7],
           [3, 5, 6],
           [3, 5, 7]])

    from http://stackoverflow.com/a/1235363/425458
    """

    arrays = [np.asarray(x) for x in arrays]
    dtype = arrays[0].dtype

    n = np.prod([x.size for x in arrays])
    if out is None:
        out = np.zeros([n, len(arrays)], dtype=dtype)

    m = n / arrays[0].size
    out[:,0] = np.repeat(arrays[0], m)
    if arrays[1:]:
        Cartesian(arrays[1:], out=out[0:m,1:])
        for j in xrange(1, arrays[0].size):
            out[j*m:(j+1)*m,1:] = out[0:m,1:]
    return out

def DiagonalIndices(arr, axes=None, offsets=None):
    '''
    get the indices along the diagonal of a multidimensional histogram
    '''
    allAxes = np.arange(len(arr.shape))
    axes = allAxes if axes is None else np.asarray(axes, dtype=np.uint)
    offsets = np.zeros(axes.size, dtype=np.int) if offsets is None else np.asarray(offsets, dtype=np.int)
    offsetDict = {ax:offset for ax,offset in zip(axes, offsets)}

    diagLen = (np.array(arr.shape, dtype=np.int)[axes] - offsets).min()
    return [np.arange(diagLen) + offsetDict[ax] if ax in offsetDict else np.zeros(diagLen, dtype=np.int) for ax in allAxes]

def DiagonalSliceIndices(arr, sliceStart, sliceEnd, axes=None):
    '''
    Returns the indices spanned by all of the diagonals that pass through
    the box with lower and upper corners defined by the points sliceStart and sliceEnd.
    '''
    rank = len(arr.shape)
    if axes is not None and not len(sliceStart)==len(sliceEnd)==len(axes):
        raise ValueError('in DiagonalSliceIndices, if axes is specific then it is required that \
                          len(sliceStart)==len(sliceEnd)==len(axes). \
                          sliceStart: %s, sliceEnd: %s, axes: %s' % (sliceStart, sliceEnd, axes))
    elif axes is None and not len(sliceStart)==len(sliceEnd)==rank:
        raise ValueError('in DiagonalSliceIndices, if axes is not specific then it is required that \
                          len(sliceStart)==len(sliceEnd)==len(arr.shape). \
                          sliceStart: %s, sliceEnd: %s, len(arr.shape): %d' % (sliceStart, sliceEnd, rank))

    sliceStart,sliceEnd = np.asarray(sliceStart, dtype=np.int),np.asarray(sliceEnd, dtype=np.int)
    sliceDims = (sliceStart - sliceEnd)
    axes = np.arange(rank) if axes is None else np.asarray(axes, dtype=np.uint)

    # based on the volume of the lower "rind" of the slice box, allocate space for offsetsArr
    rindComplementDims = sliceDims - 1
    rindVol = sliceDims.prod() - rindComplementDims.prod()
    offsetsArr = np.empty((rindVol, rank), dtype=np.int)

    # construct the indices of the lower "rind" of the slice box and store them in offsetsArr
    sliceIndicesForProduct = [[np.array([0])]]*rank
    for i,ax in enumerate(axes):
        sliceIndicesForProduct[ax] = [np.array([sliceStart[i]]), np.arange(sliceStart[i], sliceEnd[i])]
    i = 0
    for sliceIndicesForCartesian in product(*sliceIndicesForProduct):
        cartesianSize = np.prod([x.size for x in sliceIndicesForCartesian])
        Cartesian(sliceIndicesForCartesian, out=offsetsArr[i:cartesianSize])
        i+=cartesianSize

    # transform the rind indices to diagonal offsets by subtracting the smallest value in each set of indices from each set of indices
    offsetsArr-=offsetsArr.min(axis=1).reshape(-1,1)

    # feed the offsets one at a time to DiagonalIndices(), collect the results in a list-of-lists-of-arrays
    diagIArrLists = [[] for x in range(rank)]
    for offsets in offsetsArr:
        # DiagonalIndices() returns a rank-length list of 1D arrays
        for i,diagIArr in enumerate(DiagonalIndices(arr=arr, axes=axes, offsets=offsets)):
            diagIArrLists[i].append(diagIArr)

    # concatenate the list-of-lists-of-arrays down to a list-of-arrays and return it
    return [np.concatenate(diagIArrList) for diagIArrList in diagIArrLists]

def DiagonalMask(arr, sliceStart, sliceEnd, axes=None, inverse=False):
    '''
    Returns a boolean array mask that covers all of the diagonals that pass through
    the box with lower and upper corners defined by the points sliceStart and sliceEnd.
    If inverse is False, the diagonals will be covered by True values and the rest of the mask will be set to False.
    Otherwise, the diagonals will covered by False values and the rest of the mask will be set to True.
    '''
    rank = len(arr.shape)
    diagonalMask = np.zeros(arr.shape, dtype=bool)
    if rank==1:
        diagonalMask[sliceStart[0]:sliceEnd[0]] = True
    else:
        axes = np.arange(rank) if axes is None else np.asarray(axes, dtype=np.uint)

        slize,reducedSlize = [0]*rank,[0]*rank
        for i,ax in enumerate(axes):
            slize[ax] = slice(sliceStart[i], sliceEnd[i])
            reducedSlize[ax] = slice(sliceStart[i] + 1, sliceEnd[i])
        diagonalMask[slize] = 1
        diagonalMask[reducedSlize] = 0
        offsetsArr = np.column_stack(diagonalMask.nonzero())
        offsetsArr-=offsetsArr.min(axis=1).reshape(-1,1)
        for offsets in offsetsArr:
            diagonalMask[DiagonalIndices(arr=arr, axes=axes, offsets=offsets)] = True

    if inverse:
        np.logical_not(diagonalMask, diagonalMask)
    return diagonalMask

def FastStack(*args, **kwargs):
    '''
    stack compatibly shaped arrays in args into a preallocated larger array along the axis designated by the 'axis' argument in kwargs
    '''
    if len(args)==0:
        return np.array()
    
    try:
        axis = kwargs['axis']
    except KeyError:
        axis = 0
    
    stackedDtype = args[0].dtype
    stackedShape = list(args[0].shape)
    partIndices = [0, stackedShape[axis]]
    for part in args[1:]:
        stackedShape[axis]+=part.shape[axis]
        partIndices.append(stackedShape[axis])
    partIndexTuples = zip(partIndices, partIndices[1:])
        
    stacked = np.zeros(stackedShape, dtype=stackedDtype)
    for (start,end),part in zip(partIndexTuples, args):
        partSlice = []
        for i in range(len(stackedShape)):
            if i==axis:
                partSlice.append(np.s_[start:end])
            else:
                partSlice.append(np.s_[:])
        partSlice = tuple(partSlice)
        stacked[partSlice] = part
    return stacked
        
def FastHStack(*args):
    '''
    FastStack along the last axis
    '''
    axis = len(args[0].shape) - 1
    return FastStack(*args, axis=axis)
    
def FastVStack(*args):
    '''
    FastStack along the first axis
    '''
    axis = 0
    return FastStack(*args, axis=axis)