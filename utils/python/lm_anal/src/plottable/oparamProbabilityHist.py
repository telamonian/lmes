import h5py
import numpy as np
import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

from .hist import Hist
from .reducer import Reducer

class OParamProbabilityHist(Hist, Reducer):
    def __init__(self, sim, tilingID):
        self.sim = sim
        self.oparams = sim.oparams
        self.tilingID = tilingID
        self.tiling = sim.tilings[tilingID]
        
        dims = self.tiling.dims
        edges = self.tiling.edges
        rank = self.tiling.rank
        super().__init__(dims,edges,rank)
    
    def ReduceData(self):
        for datum in self.sim.replicateTrajectories.sffHDF5():
            self.AddSpeciesCounts(datum.species_count)
        
    def AddSpeciesCounts(self, speciesCounts):
        oparamVals = self.oparams[self.tiling.order_parameter_id].calc(speciesCounts)
        self.AddObs(oparamVals)