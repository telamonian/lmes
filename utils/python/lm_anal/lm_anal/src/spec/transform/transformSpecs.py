from lm_anal.src.spec.specs import Specs

__all__ = ['TransformSpecs']

class TransformSpecs(Specs):
    def genKey(self, arg, args):
        if len(args) > 1:
            return arg['name']+'_partial'
        else: 
            return arg['name']