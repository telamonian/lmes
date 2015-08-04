from lm_anal.src.spec.specs import Specs

__all__ = ['TransformSpecs']

class TransformSpecs(Specs):
    def genKey(self, spec, specList):
        if len(specList) > 1:
            return spec['name']+'_partial'
        else: 
            return spec['name']