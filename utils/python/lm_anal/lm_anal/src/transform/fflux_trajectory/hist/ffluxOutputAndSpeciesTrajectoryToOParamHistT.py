from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.transform import BaseT

class FFluxOutputAndSpeciesTrajectoryToOParamHistT(BaseT):
    srcTypes = frozenset({FFluxOutput, SpeciesTrajectory}) 
    dstTypes = frozenset({OParamHist})
    
    def __init__(self, src, dst, oparams, simParams, specTrajs, tilings, **kwargs):
        '''
        oparams: the complete oparams container
        tilings: a list of all the tilings you want to use to define the bins of the resultant histogram (OParamHist)
        specTrajs (temporary): a SpeciesTrajectories container with the data relevant to t
        '''
        super().__init__(src, dst, oparams=oparams, simParams=simParams, specTrajs=specTrajs, tilings=tilings, **kwargs)
        
    def tfd(self, srcs, dsts, keys=None, **kwargs):
        '''
        generic tfd (transform from datum) method
        '''
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        if keys==None:
            keys = srcs.keys()
            
        for key,srcDatum in zip(keys,srcs.valIter(keys)):
            # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
            dstDatum = dsts.initDatum(key, full=srcDatum.full)
            for propertyTransform in self.propertyTransforms:
                propertyTransform.transformProperties(srcDatum, dstDatum, **kwargs)