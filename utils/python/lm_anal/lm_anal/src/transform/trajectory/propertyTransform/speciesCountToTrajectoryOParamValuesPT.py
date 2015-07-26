from lm_anal.src.datum.trajectory import TrajectoryBase
from lm_anal.src.transform.propertyTransform import BasePT

class TrajectorySpeciesCountToTrajectoryOParamValuesPT(BasePT):
    srcType = TrajectoryBase
    dstType = TrajectoryBase
    srcProp = 'species_count'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparam, **kwargs):
        self.oparam = oparam
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.__setattr__(self.dstProp, self.oparam.calc(srcDatum.__getattribute__(self.srcProp)))
        except AttributeError:
            pass