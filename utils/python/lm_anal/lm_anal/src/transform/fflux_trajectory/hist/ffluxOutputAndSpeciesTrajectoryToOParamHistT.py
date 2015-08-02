from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.transform import BaseT

class FFluxOutputAndSpeciesTrajectoryToOParamHistT(BaseT):
    srcType = FFluxOutput
    dstType = OParamHist
    
    def __init__(self, src, dst, oparams, simParams, specTrajs, tilings, **kwargs):
        '''
        oparams: the complete oparams container
        tilings: a list of all the tilings you want to use to define the bins of the resultant histogram (OParamHist)
        specTrajs (temporary): a SpeciesTrajectories container with the data relevant to t
        '''
        super().__init__(src, dst, oparams=oparams, simParams=simParams, specTrajs=specTrajs, tilings=tilings, **kwargs)