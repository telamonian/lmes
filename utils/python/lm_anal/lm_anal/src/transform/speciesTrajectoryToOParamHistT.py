from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.transform import BaseT

class SpeciesTrajectoryToOParamHistT(BaseT):
    srcType = SpeciesTrajectory
    dstType = OParamHist