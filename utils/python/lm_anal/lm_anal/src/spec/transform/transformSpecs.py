from lm_anal.src.spec.transform.transformSpec import TransformSpec
from lm_anal.src.spec.specs import Specs

__all__ = ['TransformSpecs']

class TransformSpecs(Specs):
    specType = TransformSpec

    def genKey(self, spec, specList):
        if len(specList) > 1:
            return spec['name']+'_partial'
        else: 
            return spec['name']