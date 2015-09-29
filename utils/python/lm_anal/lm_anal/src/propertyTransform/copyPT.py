from itertools import product

from lm_anal.src.datumABC import GetDatumStrABCSet
from lm_anal.src.helper import Frozensetify
from lm_anal.src.propertyTransform.basePT import BasePT

class CopyPT(BasePT):
    def __init__(self, srcProps, dstProps, srcTypes, dstTypes, **kwargs):
        super().__init__(**kwargs)
        self.srcTypes = srcTypes
        self.dstTypes = dstTypes
        
        self.srcProps = Frozensetify(srcProps)
        self.dstProps = Frozensetify(dstProps)
        
    def ptfd(self, srcDict, dstDict, **kwargs):
        for srcType,dstType in product(self.srcTypes, self.dstTypes):
            for srcProp,dstProp in zip(self.srcProps, self.dstProps):
                try:
                    dstDict[dstType.__name__].__setattr__(dstProp, srcDict[srcType.__name__].__getattribute__(srcProp))
                except AttributeError:
                    pass