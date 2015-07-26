from lm_anal.src.transform.propertyTransform import PropertyTransforms

class BaseT(object):
    def __init__(self, src, dst, **kwargs):
        self.propTrans = PropertyTransforms(src.datumType, dst.datumType, **kwargs)
        self.initDst(src, dst)
        self.execTransform(src, dst)
    
    def initDst(self, src, dst):
        for key,datum in src:
            # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
            dst.initDatum(key, full=datum.full)

    def execTransform(self, src, dst):
        for key,datum in dst:
            self.propTrans(srcDatum=src[key], dstDatum=datum)