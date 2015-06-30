from src.transform.propertyTransform import propertyTransformDict
from src.transform.propertyTransform.defaultPT import DefaultPT

class PropertyTransforms(object):
    def __init__(self, src, dst, **kwargs):
        self.srcPropNames = src.datumType.propertyNames
        self.dstPropNames = dst.datumType.propertyNames
        self.ptDict = {}
        for PropTrans in propertyTransformDict[(type(src.datumType), type(dst.datumType))]:
            self.ptDict[PropTrans.dstProp] = (PropTrans(**kwargs))
        for pName in self.dstPropNames:
            if pName not in self.ptDict:
                if pName in self.srcPropNames:
                    self.ptDict[pName] = DefaultPT(srcProp=pName, dstProp=pName)
                else:
                    raise
    
    def __call__(self, srcData, dstDatum, key):
        for pt in self.ptList:
            pt(srcDatum=srcData[key], dstDatum=dstDatum)