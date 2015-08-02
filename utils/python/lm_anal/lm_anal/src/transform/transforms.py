# from importlib import import_module
# import os
# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# 
# from lm_anal.src.helper import CamelCaseUpper
# 
# transformDict = {}
# transfromDirFiles = os.walk(thisScriptDir).__next__()[2]
# modNames = (os.path.splitext(modName)[0] for modName in transfromDirFiles 
#             if (modName[-4:]=='T.py' and modName!='baseT.py'))
# for modName in modNames:
#     try:
#         tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform'), CamelCaseUpper(modName))
#     except AttributeError:
#         # TODO: fix this hackish fix
#         if CamelCaseUpper(modName)[:5]=='Fflux':
#             ModName = 'FFlux' + CamelCaseUpper(modName)[5:]
#         tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform'), ModName)
#     key = (tmpCls.srcType, tmpCls.dstType)
#     transformDict[key] = tmpCls
import os
from pathlib import Path

from lm_anal.src.datumABC import GetDatumTypeABCSet
from lm_anal.src.helper import Tupify, ShallowImportPackages

transformPath = Path(os.path.dirname(os.path.realpath(__file__)))
transformName = __package__
srcTransformPkgDict = ShallowImportPackages(path=[str(transformPath)], name=transformName, outputAll=False)
# locals().update(localDict)

class Transforms(object):
    def __init__(self, srcs, dsts, **kwargs):
        self.srcTypes = set(map(lambda x: x.datumType, Tupify(srcs)))
        self.dstTypes = set(map(lambda x: x.datumType, Tupify(dsts)))
        self.srcABCs = GetDatumTypeABCSet(self.srcTypes)
        self.dstABCs = GetDatumTypeABCSet(self.dstTypes)
        
        # based on src and dst ABCs, get the pkg with the appropriate Transform types
        for srcTransformPkg in srcTransformPkgDict.values():
            if self.srcABCs==srcTransformPkg.srcABCs:
                self.srcTransformPkg = srcTransformPkg
                break
        if not hasattr(self, 'srcTransformPkg'):
            raise
        for dstTransformPkg in self.srcTransformPkg.dstTransformPkgDict.values():
            if self.dstABCs==dstTransformPkg.dstABCs:
                self.dstTransformPkg = dstTransformPkg
                self.transformDict = self.dstTransformPkg.transformDict
                break
        if not hasattr(self, 'dstTransformPkg'):
            raise
        
#         for transformPkg in transformPkgDict.values():
#             if self.srcABCs==transformPkg.srcABCs:
#                 self.transformPkg = transformPkg
#                 break
#         if not hasattr(self, 'transformPkg'):
#             raise

        # now that we have the right transform pkg, load up the Transform itself
        for Transform in self.transformDict.values():
            if Transform.checkTypes(self.srcTypes, self.dstTypes):
                self.transform = Transform()
                break
        if not hasattr(self, 'transform'):
            raise
        self.transform.tfd(srcs, dsts, **kwargs)
        