from lm_anal.src.datumABC import datumABCDict
from lm_anal.src.propertyTransform import BasePT

__all__ = ['ToNumberOParamsPT']

class ToNumberOParamsPT(BasePT):
    srcABCs = frozenset({datumABCDict['trajectory']})
    dstABCs = frozenset({datumABCDict['trajectory']})
    
    srcProps = frozenset({''})
    dstProps = frozenset({'number_order_parameters'})
    
#     def __init__(self):#, oparam, **kwargs):
#         self.oparam = oparam
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        dstDatum.oparam = kwargs['oparams'].combineByID(kwargs['oparamIDs'])
        srcProp = next(iter(self.srcProps))
        dstProp = next(iter(self.dstProps))
        try:
            dstDatum.__setattr__(dstProp, dstDatum.oparam.numberOrderParameters)
        except AttributeError:
            pass