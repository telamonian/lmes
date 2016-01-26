from lm_anal.src.datum.hist.hist import Hist

__all__ = ['OParamHist']

class OParamHist(Hist):
    propertySpecs = {'order_parameter_values':{'targetName':'h','type':'alias'}}
    
    def __init__(self, o=None):
        super().__init__(o=o)
        
    def setTilings(self, oparams, tilings, tilingIDs):
        self.tilings = tilings.sliceByKeys(tilingIDs)
        self.oparams = oparams.sliceByKeys([tiling.order_parameter_id for tiling in self.tilings.valIter()])

        self.oparam = self.oparams.combine()
        self.tiling = self.tilings.combine()
        self.initH(dims=(self.tiling.dims + 1), edges=self.tiling.edges)

# plotting stuff
    def getXLabel(self):
        if len(self.h_dims)>1:
            return r'total A ($A+2 \cdot A_2+2 \cdot OA_2$)'
        else:
            return 'total B - total A'

    def getYLabel(self):
        if len(self.h_dims)>1:
            return r'total B ($B+2 \cdot B_2+2 \cdot OB_2$)'
        else:
            return 'counts'