import numpy as np

from lm_anal.src.datum.hist.hist import Hist

__all__ = ['OParamHist']

class OParamHist(Hist):
    propertySpecs = {'order_parameter_values':{'targetName':'h','type':'alias'}}
    
    def __init__(self, full=False):
        super().__init__(full=full)
        
    def setTilings(self, oparams, tilings, tilingIDs):
        self.tilings = tilings.sliceByKeys(tilingIDs)
        self.oparams = oparams.sliceByKeys([tiling.order_parameter_id for tiling in self.tilings.valIter()])
        #
        # dims = []
        # edges = []
        #
        #
        #
        #
        #
        # for tiling in tilings.getByID(tilingIDs):
        #     oparamIDs.append(tiling.order_parameter_id)
        #     dims.append(len(tiling.edges) + 1)
        #     edges.append(np.array(tiling.edges))


        self.oparam = self.oparams.combine()
        self.tiling = self.tilings.combine()
        # self.oparam = self.oparams[0].combine(self.oparams[1:])
        # self.tiling = self.tilings[0].combine(self.tilings[1:])
        #self.initH(dims=np.array(dims), edges=np.hstack(edges))
        self.initH(dims=(self.tiling.dims + 1), edges=self.tiling.edges)

# plotting stuff
    def getXLabel(self):
        return self.oparams[0]

    def getYLabel(self):
        if len(self.h_dims)>1:
            return self.oparams[1]
        else:
            return 'counts'