from importlib import import_module
import os
thisScriptDir = os.path.dirname(os.path.realpath(__file__))

from lm_anal.src.helper import CamelCaseUpper

transformDict = {}
transfromDirFiles = os.walk(thisScriptDir).__next__()[2]
modNames = (os.path.splitext(modName)[0] for modName in transfromDirFiles 
            if (modName[-4:]=='T.py' and modName!='baseT.py'))
for modName in modNames:
    tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform'), CamelCaseUpper(modName))
    key = (tmpCls.srcType, tmpCls.dstType)
    transformDict[key] = tmpCls

class Transforms(object):
    def __init__(self, src, dst, **kwargs):
        self.transform = transformDict[(src.datumType, dst.datumType)]
        self.execTransform(src, dst, **kwargs)
        
    def execTransform(self, src, dst, **kwargs):
        self.transform(src, dst, **kwargs)