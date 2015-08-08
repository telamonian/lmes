from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.datum.hist import FFluxHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.datumABC import datumABCDict, GetDatumTypeABC
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['FFluxOutputAndSpeciesTrajectoryToOParamHistT']

class FFluxOutputAndSpeciesTrajectoryToOParamHistT(BaseT):
    srcTypes = frozenset({FFluxOutput, SpeciesTrajectory}) 
    dstTypes = frozenset({FFluxHist})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes={FFluxOutput}, dstTypes={FFluxHist}, requiredArgs='tilingIDs', requiredData={'oparams', 'tilings'},
                                                  propertyTransformSpecs=PropertyTransformSpecs(
                                                  PropertyTransformSpec(dstProps='phase_weights', srcProps='basins', type='special'),
                                                  PropertyTransformSpec(dstProps='trajectory_phase_map', srcProps='trajectories', preMap=True, requiredArgs='tilingIDs', type='special'))),
                                    TransformSpec(srcTypes={SpeciesTrajectory}, dstTypes={FFluxHist}, requiredArgs='tilingIDs', requiredData={'oparams', 'tilings'},
                                                  propertyTransformSpecs=PropertyTransformSpecs( )))
#                                                   PropertyTransformSpec(dstProps='order_parameter_values', srcProps='species_count', requiredArgs='tilingIDs', type='special'))))
    
#     def __init__(self, src, dst, oparams, simParams, specTrajs, tilings, **kwargs):
#         '''
#         oparams: the complete oparams container
#         tilings: a list of all the tilings you want to use to define the bins of the resultant histogram (OParamHist)
#         specTrajs (temporary): a SpeciesTrajectories container with the data relevant to t
#         '''
#         super().__init__(src, dst, oparams=oparams, simParams=simParams, specTrajs=specTrajs, tilings=tilings, **kwargs)
        
    def tfd(self, srcs, dsts, keys=None, **kwargs):
        '''
        generic tfd (transform from datum) method
        '''
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        srcDataDict = {}
        for srcData in srcs:
            srcDataDict[GetDatumTypeABC(srcData.datumType)] = srcData
        
        if keys==None:
            keys = srcDataDict[datumABCDict['fflux']].keys()
            
        for key,ffluxDatum in zip(keys, srcDataDict[datumABCDict['fflux']].valIter(keys)):
            # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
            dstDatum = dsts.initDatum(key, full=ffluxDatum.full)
            srcDatumDict = {datumABCDict['fflux']:ffluxDatum, datumABCDict['trajectory']:srcDataDict[datumABCDict['trajectory']]}
            for propertyTransform in self.propertyTransforms:
                propertyTransform.transformProperties(srcDatum=srcDatumDict, dstDatum=dstDatum, **kwargs)