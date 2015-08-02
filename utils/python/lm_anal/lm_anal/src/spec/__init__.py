# from lm_anal.src.spec.spec import Spec
#  
# from lm_anal.src.spec.specs import Specs
# 
# import lm_anal.src.spec.transform as transform
#
# __all__ = {Spec, Specs} | transform.__all__

from lm_anal.src.helper import ShallowImportAllPackages
from lm_anal.src.spec.spec import Spec, SpecMetaclass
from lm_anal.src.spec.specs import Specs

__all__ = ['Spec', 'SpecMetaclass', 'Specs']

localDict, allList = ShallowImportAllPackages(path=__path__, name=__name__)
locals().update(localDict)
__all__+=allList