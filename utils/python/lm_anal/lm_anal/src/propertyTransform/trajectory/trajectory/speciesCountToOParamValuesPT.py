from lm_anal.src.datumABC import datumABCDict
from lm_anal.src.propertyTransform import BasePT

__all__ = ['SpeciesCountToTrajectoryOParamValuesPT']

class SpeciesCountToTrajectoryOParamValuesPT(BasePT):
    srcABCs = frozenset({datumABCDict['trajectory']})
    dstABCs = frozenset({datumABCDict['trajectory']})
    
    srcProps = frozenset({'species_count'})
    dstProps = frozenset({'order_parameter_values'})
    
#     def __init__(self):#, oparam, **kwargs):
#         self.oparam = oparam
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        srcProp = next(iter(self.srcProps))
        dstProp = next(iter(self.dstProps))
        srcDatum = srcDict['SpeciesTrajectory']
        dstDatum = dstDict['OParamTrajectory']
        
        dstDatum.oparam = kwargs['oparams'].combineByID(kwargs['oparamIDs'])
        try:
            dstDatum.__setattr__(dstProp, dstDatum.oparam.calc(srcDatum.__getattribute__(srcProp)))
        except AttributeError:
            pass