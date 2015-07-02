from lm_anal.src.transform.propertyTransform import BasePT

class DefaultPT(BasePT):
    def __init__(self, srcProp, dstProp, **kwargs):
        super().__init__(**kwargs)
        self.srcProp = srcProp
        self.dstProp = dstProp
        
    def __call__(self, srcDatum, dstDatum):
#         if srcDatum.propertySpecs[self.srcName].full
        try:
            dstDatum.__setattr__(self.dstProp, srcDatum.__getattribute__(self.srcProp))
        except AttributeError:
            pass