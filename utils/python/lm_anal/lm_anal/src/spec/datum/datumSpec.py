from lm_anal.src.spec import Spec

__all__ = ['DatumSpec']

class DatumSpec(Spec): #, metaclass=DatumPropertySpecMetaclass):
    keywords = {'dtype', 'name', 'paths', 'storageType', 'targetField', 'targetName', 'type'}
    
    conditionalKeywords = [{'checkKeyword':'type','equals':'subData', 'keywords':'SubDataType'}]

                           # another (deprecated) example of a conditionalKeyword
                           #{'checkKeyword':'type','equals':'array', 'keywords':'columnLabels'}

    defaultKeywordDict = {'type': None}