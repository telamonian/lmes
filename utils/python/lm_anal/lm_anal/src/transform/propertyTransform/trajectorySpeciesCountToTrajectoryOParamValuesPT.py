from lm_anal.src.transform.propertyTransform.propertyTransform import PropertyTransform

class TrajectorySpeciesCountToTrajectoryOParamValuesPT(PropertyTransform):
    srcType = PropertyTransform.Trajectory
    dstType = PropertyTransform.Trajectory
    srcProp = 'species_count'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparam, **kwargs):
        self.oparam = oparam
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.__setattr__(self.dstProp, self.oparam.calc(srcDatum.__getattribute__(self.srcProp)))
        except AttributeError:
            pass