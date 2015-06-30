from lm_anal.src.transform.propertyTransform.propertyTransform import PropertyTransform

class DefaultPT(PropertyTransform):
    def __init__(self, srcName, dstName, **kwargs):
        super.__init__(**kwargs)
        self.srcName = srcName
        self.dstName = dstName
        
    def __call__(self, srcDatum, dstDatum):
        dstDatum.__setattr__(self.dstName, srcDatum.__getattribute__(self.srcName))