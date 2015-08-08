import numpy as np

__all__ = ['FastStack', 'FastHStack', 'FastVStack']

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