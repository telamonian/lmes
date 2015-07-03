import numpy as np

from lm_anal.src.datum.hist import Hist

class OParamHist(Hist):
    propertySpecs = {'order_parameter_values':{'targetName':'h','type':'alias'}}
    
    def __init__(self, full=False):
        super().__init__(full=full)
        
    def setTilings(self, oparams, tilings):
        self.oparams = []
        dims = []
        edges = []
        for tiling in tilings:
            self.oparams.append(oparams[tiling.order_parameter_id])
            dims.append(len(tiling.edges) + 1)
            edges.append(np.array(tiling.edges))
        self.dims = np.array(dims)
        self.edges = np.hstack(edges)
        self.oparam = self.oparams[0].combine(self.oparams[1:])