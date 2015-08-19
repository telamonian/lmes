from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.spec.spec import Spec

__all__ = ['PropertyTransformSpec']

class PropertyTransformSpec(Spec):
    keywords = {'name', 'dstProps', 'srcProps', 'preMap', 'requiredArgs', 'type'}
    
    requiredKeywords = {'dstProps', 'srcProps'}
    setKeywords = {'dstProps', 'srcProps', 'requiredArgs'}
    
    defaultKeywordDict = {'type':'special'}
#     defaultKeywordDict = {'preMap':False}
    
    def defaultName(self, **kwargs):
        # if we don't specify a name, come up with one from srcProps + dstProps -> <SP1>And<SP2>...To<DP1>And<DP2>...
        return 'To'.join(('And'.join(map(CamelCaseUpper, kwargs['srcProps'])), 
                         'And'.join(map(CamelCaseUpper, kwargs['dstProps']))))