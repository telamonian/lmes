from src.datum.trajectory.trajectory import Trajectory

class PropertyTransform(object):
    Trajectory = Trajectory
    dataTypeList = [Trajectory]
    
    srcType = None
    dstType = None
    srcProp = None
    dstProp = None
    
    def __init__(self, **kwargs):
        pass
    
    def __call__(self, srcDatum, dstDatum):
        pass