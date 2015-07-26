from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.spec.spec import Spec

class PropertyTransformSpec(Spec):
    keywords = {'name',  'srcProps', 'dstProps', 'keywords', 'type'}
    
    requiredKeywords = {'srcProps', 'dstProps'}
    tupleKeywords = {'srcProps', 'dstProps'}
    
    def defaultName(self, **kwargs):
        # if we don't specify a name, come up with one from srcProps + dstProps -> <SP1>And<SP2>...To<DP1>And<DP2>...
        return 'To'.join('And'.join(CamelCaseUpper(kwargs['srcProps'])), 'And'.join(CamelCaseUpper(kwargs['dstProps'])))