import importlib
import os
thisScriptDir = os.path.dirname(os.path.realpath(__file__))

from src.helper import CamelCaseUpper

propertyTransformList = []
propertyTransfromDirFiles = os.walk(thisScriptDir).next()[2]
modNames = (os.path.splitext(modName)[0] for modName in propertyTransfromDirFiles 
            if (modName[-5:]=='PT.py' and modName!='defaultPT.py'))
for modName in modNames:
    propertyTransformList.append(getattr(__import__(modName), CamelCaseUpper(modName)))
#     tmpCls = getattr(__import__(modName), CamelCaseUpper(modName))
#     key = (tmpCls.srcType, tmpCls.dstType)
#     propertyTransformDict[key] = propertyTransformDict.get(key, []) + [tmpCls]