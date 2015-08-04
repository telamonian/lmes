from lm_anal.src.helper import Frozensetify
from lm_anal.src.propertyTransform.basePT import BasePT

class CopyPT(BasePT):
    def __init__(self, srcProps, dstProps, **kwargs):
        super().__init__(**kwargs)
        self.srcProps = Frozensetify(srcProps)
        self.dstProps = Frozensetify(dstProps)
        
    def ptfd(self, srcDatum, dstDatum, **kwargs):
#         if srcDatum.propertySpecs[self.srcName].full
        for srcProp,dstProp in zip(self.srcProps, self.dstProps):
            try:
                dstDatum.__setattr__(dstProp, srcDatum.__getattribute__(srcProp))
            except AttributeError:
                pass