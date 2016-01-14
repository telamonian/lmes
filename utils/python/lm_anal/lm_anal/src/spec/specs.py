from collections import OrderedDict
from copy import deepcopy

__all__ = ['SpecsMetaclass', 'Specs']

def DefAllProp(setKeyword):
    @property
    def prop(self):
        return set().union(*(spec[setKeyword] for spec in self.values()))
    return prop
 
class SpecsMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        if 'specType' in dct:
            for setKeyword in dct['specType'].setKeywords:
                # for every setKeyword in this Specs's specType, create a property that returns the union of all corresponding sets contained in a Specs instance
                dct['%sAll' % setKeyword] = DefAllProp(setKeyword)
        return super(SpecsMetaclass, cls).__new__(cls, clsname, bases, dct)

class Specs(object, metaclass=SpecsMetaclass):
    def __init__(self, *specList, **kwargs):
        # init attributes directly from args
        self.keywordValOverrideDict = kwargs['keywordValOverrideDict'] if 'keywordValOverrideDict' in kwargs else None

        # init internal attrs
        self.counter = 0
        self.map = OrderedDict()

        # init self.map
        self.addSpecList(specList)

    def __contains__(self, key):
        return key in self.map
        
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val
    
    def __iter__(self):
        return self.map.__iter__()
    
    def addSpec(self, spec, specList=None):
        '''
        Even when adding a single spec, the whole specList may affect how its key is generated, so make a way to pass it in
        '''
        specKey = self.genKey(spec=spec, specList=specList)
        self[specKey] = spec
        if self.keywordValOverrideDict is not None:
            for keyword,val in self.keywordValOverrideDict.items():
                self[specKey][keyword] = val

    def addSpecList(self, specList):
        for spec in specList:
            self.addSpec(spec=spec, specList=specList)
    
    def combine(self, *others):
        newSpecs = deepcopy(self)
        newSpecs.update(others)
        return newSpecs

    def defaultKey(self, spec, specList):
        while self.counter in self:
            self.counter+=1
        return self.counter
    
    def genKey(self, spec, specList):
        if 'name' in spec:
            return spec['name']
        else:
            return self.defaultKey(spec, specList)

    def getCopy(self):
        return deepcopy(self)

    def items(self):
        return self.map.items()

    def keys(self):
        return self.map.keys()

    def setKeywordVal(self, keyword, val, specKeys=None):
        '''
        overwrites the val of keyword in self[specKeys]. If specKeys is none, this is run on every Spec contained in this Specs instance
        '''
        if specKeys is None:
            specKeys = list(self.keys())

        for specKey in specKeys:
            self.setKeywordValBySpecKey(keyword=keyword, val=val, specKey=specKey)

    def setKeywordValBySpecKey(self, keyword, val, specKey):
        self[specKey][keyword] = val

    def size(self):
        return len(self.map)

    def update(self, *others):
        for other in others:
            self.map.update(other.map)

    def values(self):
        return self.map.values()