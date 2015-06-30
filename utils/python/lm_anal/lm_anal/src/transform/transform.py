from lm_anal.src.transform.propertyTransform.propertyTransforms import PropertyTransforms

class Transform(object):
    def __init__(self, src, dst, **kwargs):
        self.propTrans = PropertyTransforms(src, dst, **kwargs)
        self.do(src, dst)
    
    def do(self, src, dst):
        for key,datum in src:
            dst.initDatum(key)
        for key,datum in dst:
            self.propTrans(srcData=src, dstDatum=datum, key=key)
        
        
#     def tfd(self, srcData, dstDatum, key):
#         for prop in srcData[key].propertySpecs.keys():
#             
#     
#     def _tfd(self, src, dst, **kwargs):
#         '''
#         tfd: transform from data
#         '''
#         pass