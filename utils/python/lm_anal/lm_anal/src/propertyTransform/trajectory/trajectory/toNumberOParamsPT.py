from lm_anal.src.propertyTransform import BasePT

__all__ = ['ToNumberOParamsPT']

class ToNumberOParamsPT(BasePT):
    #srcTypes = {TrajectoryBase}
#     dstABCs = {TrajectoryBase}
    srcProps = {''}
    dstProps = {'number_order_parameters'}
    
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