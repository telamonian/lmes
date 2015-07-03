from lm_anal.src.datum.hist import HistBase
from lm_anal.src.datum.trajectory import TrajectoryBase
from lm_anal.src.transform.propertyTransform import BasePT

class TrajectorySpeciesCountToHistOParamValuesPT(BasePT):
    srcType = TrajectoryBase
    dstType = HistBase
    srcProp = 'species_count'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparams, tilings, **kwargs):
        self.oparams = oparams
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            dstDatum.setArray(self.dstProp, self.oparam.calc(srcDatum.__getattribute__(self.srcProp)))
        except AttributeError:
            pass