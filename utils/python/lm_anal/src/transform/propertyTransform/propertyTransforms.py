from src.transform.propertyTransform import propertyTransformDict
from src.transform.propertyTransform.defaultPT import DefaultPT
from src.transform.propertyTransform.propertyTransform import PropertyTransform

class PropertyTransforms(object):
    def __init__(self, src, dst, **kwargs):
        self.srcPropNames = src.datumType.propertyNames
        self.dstPropNames = dst.datumType.propertyNames
        self.ptDict = {}
        for PropTrans in propertyTransformDict[(self.getBaseClass(src.datumType), self.getBaseClass(dst.datumType))]:
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
            
    def getBaseClass(self, datumType):
        if isinstance(datumType, PropertyTransform.Trajectory):
            return PropertyTransform.Trajectory
        else:
            raise