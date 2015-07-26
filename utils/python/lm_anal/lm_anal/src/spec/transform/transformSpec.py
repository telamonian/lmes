from lm_anal.src.spec.spec import Spec

class TransformSpec(Spec):
    keywords = {'name', 'keywords'}
    
    requiredKeywords = {'srcTypes', 'dstTypes', 'propTransSpecs'}
    # if we got a single srcTypes/dstTypes rather than a list (or whatever), put it in a tuple. Otherwise, convert to tuple
    tupleKeywords = {'srcTypes', 'dstTypes'}
        
    def defaultName(self, **kwargs):
        # if we don't specify a name, come up with one from srcTypes + dstTypes -> <ST1>And<ST2>...To<DT1>And<DT2>...
        return 'To'.join('And'.join(kwargs['srcTypes']), 'And'.join(kwargs['dstTypes']))