from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.transform import BaseT

class SpeciesTrajectoryToOParamTrajectoryT(BaseT):
    srcType = SpeciesTrajectory
    dstType = OParamTrajectory
    
    def __init__(self, src, dst, oparam, **kwargs):
        self.oparam = oparam
        super().__init__(src, dst, oparam=oparam, **kwargs)

    def execTransform(self, src, dst):
        for key,datum in dst:
            self.propTrans(srcDatum=src[key], dstDatum=datum)
            datum.setScalar('number_order_parameters', self.oparam.numberOrderParameters)