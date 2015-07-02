from lm_anal.src.datum import Datum

class OParamHist(Datum):
    propertySpecs = {'order_parameter_values':{'targetName':'h','type':'alias'}}
    
    def __init__(self, full=False):
        super().__init__(full=full)
        
    def setTiling(self, tiling, oparams):
        self.oparam = oparams[tiling.order_parameter_id]
        self.edges = tiling.edges