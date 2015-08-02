from lm_anal.src.datumABC import datumABCDict
from lm_anal.src.helper import  ShallowImportPackages

srcABCs = {datumABCDict['fflux']}

dstPropertyTransformPkgDict, __all__ = ShallowImportPackages(path=__path__, name=__name__)
locals().update(dstPropertyTransformPkgDict)