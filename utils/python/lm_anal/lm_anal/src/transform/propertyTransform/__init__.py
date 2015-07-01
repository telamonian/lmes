from importlib import import_module
import os
thisScriptDir = os.path.dirname(os.path.realpath(__file__))

from lm_anal.src.helper import CamelCaseUpper

propertyTransformDict = {}
propertyTransfromDirFiles = os.walk(thisScriptDir).__next__()[2]
modNames = (os.path.splitext(modName)[0] for modName in propertyTransfromDirFiles 
            if (modName[-5:]=='PT.py' and modName!='defaultPT.py'))
for modName in modNames:
    tmpCls = getattr(import_module('.'+modName, package='lm_anal.src.transform.propertyTransform'), CamelCaseUpper(modName))
    key = (tmpCls.srcType, tmpCls.dstType)
    propertyTransformDict[key] = propertyTransformDict.get(key, []) + [tmpCls]