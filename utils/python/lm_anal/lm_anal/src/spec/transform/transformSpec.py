from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.spec.spec import Spec

__all__ = ['TransformSpec']

class TransformSpec(Spec):
    keywords = {'name', 'srcTypes', 'dstTypes', 'propertyTransformSpecs', 'extraArgs', 'requiredData'}
    
    requiredKeywords = {'srcTypes', 'dstTypes'}
    # if we got a single srcTypes/dstTypes rather than a list (or whatever), put it in a set. Otherwise, convert to set
    setKeywords = {'srcTypes', 'dstTypes', 'extraArgs', 'requiredData'}
        
    def defaultName(self, **kwargs):
        # if we don't specify a name, come up with one from srcTypes + dstTypes -> <ST1>And<ST2>...To<DT1>And<DT2>...
        return 'To'.join(('And'.join(map(lambda x: CamelCaseUpper(x.__name__), kwargs['srcTypes'])), 
                         'And'.join(map(lambda x: CamelCaseUpper(x.__name__), kwargs['dstTypes']))))
#         return 'To'.join('And'.join(kwargs['srcTypes']), 'And'.join(kwargs['dstTypes']))