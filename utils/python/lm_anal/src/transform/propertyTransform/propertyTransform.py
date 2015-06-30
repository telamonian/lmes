from src.datum.trajectory.trajectory import TrajectoryBase

class PropertyTransform(object):
    TrajectoryType = TrajectoryBase
    
    srcType = None
    dstType = None
    srcProp = None
    dstProp = None
    
    def __init__(self, **kwargs):
        pass
    
    def __call__(self, srcDatum, dstDatum):
        pass