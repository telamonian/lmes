import numpy as np

from .datumError import *

class arrayDatum(object):
    def __init__(self, dims=None, labels=None, rank=None):
        if dims!=None and rank==None:
            self.dims = np.array(dims, dtype=int)
            self.rank = dims.shape[0]
        elif dims==None and rank!=None:
            self.dims = np.zeros((rank,), dtype=int)
            self.rank = rank
        elif dims==None and rank==None:
            self.dims = np.zeros((0,), dtype=int)
            self.rank = 0
        else:
            if len(dims)!=rank:
                raise dimensioningError(dims, rank)
            self.dims = np.array(dims, dtype=int)
            self.rank = rank