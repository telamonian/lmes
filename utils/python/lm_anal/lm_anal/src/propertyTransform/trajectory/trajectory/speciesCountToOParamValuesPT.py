from lm_anal.src.propertyTransform import BasePT

__all__ = ['SpeciesCountToTrajectoryOParamValuesPT']

class SpeciesCountToTrajectoryOParamValuesPT(BasePT):
    #srcTypes = {TrajectoryBase}
#     dstABCs = {TrajectoryBase}
    srcProps = {'species_count'}
    dstProps = {'order_parameter_values'}
    
#     def __init__(self):#, oparam, **kwargs):
#         self.oparam = oparam
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        dstDatum.oparam = kwargs['oparams'].combineByID(kwargs['oparamIDs'])
        srcProp = next(iter(self.srcProps))
        dstProp = next(iter(self.dstProps))
        try:
            dstDatum.__setattr__(dstProp, dstDatum.oparam.calc(srcDatum.__getattribute__(srcProp)))
        except AttributeError:
            pass