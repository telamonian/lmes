from lm_anal.src.propertyTransform.basePT import BasePT

class CopyPT(BasePT):
    def __init__(self, srcProp, dstProp, **kwargs):
        super().__init__(**kwargs)
        self.srcProp = srcProp
        self.dstProp = dstProp
        
    def ptfd(self, srcDatum, dstDatum, **kwargs):
#         if srcDatum.propertySpecs[self.srcName].full
        try:
            dstDatum.__setattr__(self.dstProp, srcDatum.__getattribute__(self.srcProp))
        except AttributeError:
            pass