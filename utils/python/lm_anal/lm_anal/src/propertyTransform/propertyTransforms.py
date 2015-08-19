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

from lm_anal.src.datumABC import GetDatumTypeABCSet
from lm_anal.src.helper import FindInstanceInSet, Setify, ShallowImportPackages
from lm_anal.src.propertyTransform.copyPT import CopyPT

propertyTransformPath = Path(os.path.dirname(os.path.realpath(__file__)))
propertyTransformName = __package__
srcPropertyTransformPkgDict = ShallowImportPackages(path=[str(propertyTransformPath)], name=propertyTransformName, outputAll=False)

__all__ = ['PropertyTransforms']

class PropertyTransforms(object):
    def __init__(self, srcTypes, dstTypes, propertyTransformSpecs, srcDataTypes=None, dstDataTypes=None, srcKeyType=None, dstKeyType=None, **kwargs):
        self.propertyTransforms = []
        self.srcTypes = srcTypes
        self.dstTypes = dstTypes
        self.srcABCs = GetDatumTypeABCSet(self.srcTypes)
        self.dstABCs = GetDatumTypeABCSet(self.dstTypes)
        self.propertyTransformSpecs = propertyTransformSpecs
        
        # settings the attrs that might be ==None
        for attrName,val in zip(('srcDataTypes', 'dstDataTypes'), (srcDataTypes, dstDataTypes)):
            if val!=None:
                self.__setattr__(attrName, val)
            else:
                self.__setattr__(attrName, set())
        for attrName,val in zip(('srcKeyType', 'dstKeyType'), (srcKeyType, dstKeyType)):
            if val!=None:
                self.__setattr__(attrName, val)
            else:
                self.__setattr__(attrName, None)
        
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
            if propTransSpec.type=='copy':
                self.propertyTransforms.append(CopyPT(srcProps=propTransSpec.srcProps, dstProps=propTransSpec.dstProps))
            elif propTransSpec.type=='special':
                ptFound = False
                for PropertyTransform in self.propertyTransformDict.values():
                    if PropertyTransform.checkProps(PropertyTransform, propTransSpec.srcProps, propTransSpec.dstProps):
                        self.propertyTransforms.append(PropertyTransform())
                        ptFound = True
                        break
                if not ptFound:
                    raise
            else:
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
    
    @staticmethod
    def findDataFromDatumInSet(datumSet, Tipe):
        for obj in datumSet:
            try:
                if issubclass(obj.datumType, Tipe):
                    return obj
            except AttributeError:
                pass
        # we got here because no appropriate data instance was found
        raise
    
    def buildDict(self, datumSet, kind='src'):
        retDict = {}
        datumTypes = self.__getattribute__('%sTypes' % kind)
        dataTypes = self.__getattribute__('%sDataTypes' % kind)
        for DatumType in datumTypes:
            retDict[DatumType.__name__] = FindInstanceInSet(datumSet, DatumType)
        for DataType in dataTypes:
            retDict[DataType.__name__] = self.findDataFromDatumInSet(datumSet, DataType)
        return retDict
    
    def transformProperties(self, srcs, dsts, **kwargs):
        srcDict = self.buildDict(srcs, kind='src')
        dstDict = self.buildDict(dsts, kind='dst')
        for propTran in self.propertyTransforms:
            propTran.ptfd(srcDict=srcDict, dstDict=dstDict, **kwargs)
            
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