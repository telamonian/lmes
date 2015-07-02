from abc import ABCMeta

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
    def dims(self):
        dims = ()
        for edges in self.edgess:
            dims+=edges.shape + 1
    
    @property
    def rank(self):
        return len(self.dims.shape)
    
    def Init(self):
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
        self.h+=histogramdd(obs, bins=self.edges)[0]
    
HistBase.register(Hist)