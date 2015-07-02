from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.transform.propertyTransform.propertyTransforms import PropertyTransforms
from lm_anal.src.transform.transform import Transform

class SpeciesTrajectoryToOParamTrajectoryT(Transform):
    srcType = SpeciesTrajectory
    dstType = OParamTrajectory
    
    def __init__(self, src, dst, oparam, **kwargs):
        self.oparam = oparam
        super().__init__(src, dst, oparam=oparam, **kwargs)
    
    def initDst(self, src, dst):
        for key,datum in src:
            dst.initDatum(key)

    def execTransform(self, src, dst):
        for key,datum in dst:
            self.propTrans(srcDatum=src[key], dstDatum=datum)
            datum.setScalar('number_order_parameters', self.oparam.numberOrderParameters)