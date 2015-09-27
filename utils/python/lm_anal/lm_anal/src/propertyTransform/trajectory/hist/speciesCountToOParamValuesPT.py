from lm_anal.src.datumABC import datumABCDict
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['SpeciesCountToHistOParamValuesPT']

class SpeciesCountToHistOParamValuesPT(BasePT):
    srcABCs = frozenset({datumABCDict['trajectory']})
    dstABCs = frozenset({datumABCDict['hist']})
    
    srcProps = frozenset({'species_count'})
    dstProps = frozenset({'order_parameter_values'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        srcDatum = srcDict['SpeciesTrajectory']
        dstDatum = dstDict['OParamHist']
        if not dstDatum.full:
            return
        
        srcProp = next(iter(self.srcProps))
        dstProp = next(iter(self.dstProps))
        
        dstDatum.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
        dstDatum.setObservations(dstDatum.oparam.calc(srcDatum.__getattribute__(srcProp)))
    
#     def __init__(self, oparams, tilings, **kwargs):
#         self.oparams = oparams
#         self.tilings = tilings
    
#     def __call__(self, srcDatum, dstDatum):
#         try:
#             dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
#             dstDatum.setObservations(dstDatum.oparam.calc(srcDatum.__getattribute__(self.srcProp)))
#         except AttributeError:
#             pass