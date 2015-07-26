from lm_anal.src.spec.specs import Specs

class TransformSpecs(Specs):
    def genKey(self, args, arg):
        if len(args) > 1:
            return arg['name']+'_partial'
        else: 
            return arg['name']