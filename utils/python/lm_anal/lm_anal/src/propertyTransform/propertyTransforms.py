##################################################
# auto-import magic
##################################################
# from importlib import import_module
# import os
# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# 
# from lm_anal.src.helper import CamelCaseUpper
# 
# propertyTransformDict = {}
# propertyTransfromDirFiles = os.walk(thisScriptDir).__next__()[2]
# modNames = (os.path.splitext(modName)[0] for modName in propertyTransfromDirFiles 
#             if (modName[-5:]=='PT.py' and not (modName=='defaultPT.py' or modName=='basePT.py')))
# for modName in modNames:
#     # TODO: fix this hackish fix
#     className = CamelCaseUpper(modName)
#     if className[:5]=='Fflux':
#         className = 'FFlux' + className[5:]
#         
#     tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform.propertyTransform'), className)
#     key = (tmpCls.srcType, tmpCls.dstType)
#     propertyTransformDict[key] = propertyTransformDict.get(key, []) + [tmpCls]
##################################################
##################################################
import os
from pathlib import Path

from lm_anal.src.helper import ShallowImportPackages

propertyTransformPath = Path(os.path.dirname(os.path.realpath(__file__)))
propertyTransformName = __package__
srcPropertyTransformPkgDict = ShallowImportPackages(path=[str(propertyTransformPath)], name=propertyTransformName, outputAll=False)

# from lm_anal.src.transform.propertyTransform.copyPT import CopyPT

__all__ = ['PropertyTransforms']

class PropertyTransforms(object):
    def __init__(self, srcABCs, dstABCs, propertyTransformSpecs):
        self.propertyTransforms = []
        self.srcABCs = srcABCs
        self.dstABCs = dstABCs
        self.propertyTransformSpecs = propertyTransformSpecs
        
        # based on src and dst ABCs, get the pkg with the appropriate PropertyTransform types
        for srcPropertyTransformPkg in srcPropertyTransformPkgDict.values():
            if self.srcABCs==srcPropertyTransformPkg.srcABCs:
                self.srcPropertyTransformPkg = srcPropertyTransformPkg
                break
        if not hasattr(self, 'srcPropertyTransformPkg'):
            raise
        for dstPropertyTransformPkg in self.srcPropertyTransformPkg.dstPropertyTransformPkgDict.values():
            if self.dstABCs==dstPropertyTransformPkg.dstABCs:
                self.dstPropertyTransformPkg = dstPropertyTransformPkg
                self.propertyTransformDict = self.dstPropertyTransformPkg.propertyTransformDict
                break
        if not hasattr(self, 'dstPropertyTransformPkg'):
            raise
        
        # now that we have the right pt pkg, load up the pts themselves
        for propTransSpec in self.propertyTransformSpecs.values():
            ptFound = False
            for PropertyTransform in self.propertyTransformDict.values():
                if PropertyTransform.checkProps(propTransSpec.srcProps, propTransSpec.dstProps):
                    self.propertyTransforms.append(PropertyTransform())
                    ptFound = True
                    break
            if not ptFound:
                raise
            
#         self.srcPropNames = srcDatumType.propertyNames
#         self.dstPropNames = dstDatumType.propertyNames
#         self.ptDict = {}
#         for PropTrans in propertyTransformDict[(self.getBaseClass(srcDatumType), self.getBaseClass(dstDatumType))]:
#             self.ptDict[PropTrans.dstProp] = (PropTrans(**kwargs))
#         for pName in self.dstPropNames:
#             if pName not in self.ptDict:
#                 if pName in self.srcPropNames:
#                     self.ptDict[pName] = DefaultPT(srcProp=pName, dstProp=pName)
#                 else:
#                     raise
    
    def transformProperties(self, srcDatum, dstDatum, **kwargs):
        for propTran in self.propertyTransforms:
            propTran.ptfd(srcDatum=srcDatum, dstDatum=dstDatum, **kwargs)
            
#     def getBaseClass(self, datumType):
#         '''
#         get the abstract base class for a datum type
#         '''
# #         if PropertyTransform.Trajectory in datumType.__mro__:
#         if issubclass(datumType, FFluxBase):
#             return FFluxBase
#         elif issubclass(datumType, HistBase):
#             return HistBase
#         elif issubclass(datumType, TrajectoryBase):
#             return TrajectoryBase
#         # add base classes to this if-else clause as I make them
#         else:
#             raise