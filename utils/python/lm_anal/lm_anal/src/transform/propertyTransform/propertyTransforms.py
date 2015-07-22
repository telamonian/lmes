##################################################
# auto-import magic
##################################################
from importlib import import_module
import os
thisScriptDir = os.path.dirname(os.path.realpath(__file__))

from lm_anal.src.helper import CamelCaseUpper

propertyTransformDict = {}
propertyTransfromDirFiles = os.walk(thisScriptDir).__next__()[2]
modNames = (os.path.splitext(modName)[0] for modName in propertyTransfromDirFiles 
            if (modName[-5:]=='PT.py' and not (modName=='defaultPT.py' or modName=='basePT.py')))
for modName in modNames:
    # TODO: fix this hackish fix
    className = CamelCaseUpper(modName)
    if className[:5]=='Fflux':
        className = 'FFlux' + className[5:]
        
    tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform.propertyTransform'), className)
    key = (tmpCls.srcType, tmpCls.dstType)
    propertyTransformDict[key] = propertyTransformDict.get(key, []) + [tmpCls]
##################################################
##################################################

from lm_anal.src.datum.fflux import FFluxBase
from lm_anal.src.datum.hist import HistBase
from lm_anal.src.datum.trajectory import TrajectoryBase
from lm_anal.src.transform.propertyTransform.defaultPT import DefaultPT

class PropertyTransforms(object):
    def __init__(self, srcDatumType, dstDatumType, **kwargs):
        self.srcPropNames = srcDatumType.propertyNames
        self.dstPropNames = dstDatumType.propertyNames
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
        if issubclass(datumType, FFluxBase):
            return FFluxBase
        elif issubclass(datumType, HistBase):
            return HistBase
        elif issubclass(datumType, TrajectoryBase):
            return TrajectoryBase
        # add base classes to this if-else clause as I make them
        else:
            raise