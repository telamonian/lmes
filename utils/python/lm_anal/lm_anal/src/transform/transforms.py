from lm_anal.src.transform import transformDict
from lm_anal.src.transform.propertyTransform.defaultPT import DefaultPT
from lm_anal.src.trajectory.trajectory import TrajectoryBase

class Transforms(object):
    def __init__(self, src, dst, **kwargs):
        
        self.ptDict = {}
        for PropTrans in propertyTransformDict[(self.getBaseClass(srcDatumType), self.getBaseClass(dstDatumType))]:
            self.ptDict[PropTrans.dstProp] = (PropTrans(**kwargs))
        for pName in self.dstPropNames:
            if pName not in self.ptDict:
                if pName in self.srcPropNames:
                    self.ptDict[pName] = DefaultPT(srcProp=pName, dstProp=pName)
#                 else:
#                     raise
    
    def __call__(self, srcDatum, dstDatum):
        for pt in self.ptDict.values():
            pt(srcDatum=srcDatum, dstDatum=dstDatum)
            
    def getBaseClass(self, datumType):
        '''
        get the abstract base class for a datum type
        '''
#         if PropertyTransform.Trajectory in datumType.__mro__:
        if issubclass(datumType, TrajectoryBase):
            return TrajectoryBase
        # add base classes to this if-else clause as I make them
        else:
            raise