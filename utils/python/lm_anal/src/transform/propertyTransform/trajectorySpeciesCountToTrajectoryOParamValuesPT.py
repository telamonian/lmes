from src.datum.trajectory.trajectory import TrajectoryBase
from src.transform.propertyTransform.propertyTransform import PropertyTransform

class TrajectorySpeciesCountToTrajectoryOParamValuesPT(PropertyTransform):
    srcType = TrajectoryBase
    dstType = TrajectoryBase
    srcProp = 'species_count'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparam, **kwargs):
        self.oparam = oparam
    
    def __call__(self, srcDatum, dstDatum):
        pass