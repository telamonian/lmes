import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

import h5py
from .hist import Hist
import numpy as np

class OParamProbabilityHist(Hist):
    def __init__(self, oparams, tiling):
        self.oparams = oparams
        self.tiling = tiling
        
        dims = self.tiling.dims
        edges = self.tiling.edges
        rank = self.tiling.rank
        super().__init__(dims,edges,rank)
    
    def AddSpeciesCounts(self, speciesCounts):
        oparamVals = self.oparams[self.tiling.order_parameter_id].calc(speciesCounts)
        self.AddObs(oparamVals)