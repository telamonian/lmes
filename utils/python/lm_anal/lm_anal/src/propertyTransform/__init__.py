from lm_anal.src.propertyTransform.basePT import BasePT
from lm_anal.src.propertyTransform.copyPT import CopyPT
from lm_anal.src.propertyTransform.propertyTransforms import PropertyTransforms

__all__ = ['BasePT','CopyPT','PropertyTransforms']



# import os
# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# 
# def PTImportMagic(thisSciptDir):
#     from importlib import import_module
#     
#     from lm_anal.src.helper import CamelCaseUpper
#     
#     propertyTransformDict = {}
#     propertyTransfromDirFiles = os.walk(thisScriptDir).__next__()[2]
#     modNames = (os.path.splitext(modName)[0] for modName in propertyTransfromDirFiles 
#                 if (modName[-5:]=='PT.py' and not (modName=='defaultPT.py' or modName=='basePT.py')))
#     for modName in modNames:
#         # TODO: fix this hackish fix
#         className = CamelCaseUpper(modName)
#         if className[:5]=='Fflux':
#             className = 'FFlux' + className[5:]
#             
#         tmpCls = getattr(import_module('.'+modName, package=__package__), className)   # package='lm_anal.src.transform.propertyTransform'
#         key = (tmpCls.srcType, tmpCls.dstType)
#         propertyTransformDict[key] = propertyTransformDict.get(key, []) + [tmpCls]