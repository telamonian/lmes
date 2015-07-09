from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.transform import BaseT

class SpeciesTrajectoryToOParamHistT(BaseT):
    srcType = SpeciesTrajectory
    dstType = OParamHist
    
    def __init__(self, src, dst, oparams, tilings, **kwargs):
        '''
        oparams: the complete oparams container
        tilings: a list of all the tilings you want to use to define the bins of the resultant histogram (OParamHist)
        '''
        super().__init__(src, dst, oparams=oparams, tilings=tilings, **kwargs)