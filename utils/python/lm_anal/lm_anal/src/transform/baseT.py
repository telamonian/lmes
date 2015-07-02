from lm_anal.src.transform.propertyTransform import PropertyTransforms

class BaseT(object):
    def __init__(self, src, dst, **kwargs):
        self.propTrans = PropertyTransforms(src.datumType, dst.datumType, **kwargs)
        self.initDst(src, dst)
        self.execTransform(src, dst)
    
    def initDst(self, src, dst):
        for key,datum in src:
            dst.initDatum(key)

    def execTransform(self, src, dst):
        for key,datum in dst:
            self.propTrans(srcDatum=src[key], dstDatum=datum)