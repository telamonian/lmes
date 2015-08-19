from collections import OrderedDict

from lm_anal.src.helper import Setify,Tupify

__all__ = ['Spec','SpecMetaclass']

def DefProp(name):
    @property
    def prop(self):
        return self[name]
    @prop.setter
    def prop(self, val):
        self[name] = val
    return prop

class KeywordDescriptor(object):
    def __init__(self, name):
        self.name = name
    
    def __get__(self, obj, objtype):
        return obj[self.name]

    def __set__(self, obj, val):
        obj[self.name] = val

class SpecMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        for keyword in (dct.get('keywords', set()) |
                        dct.get('requiredKeywords', set()) |
                        dct.get('setKeywords', set()) |
                        dct.get('tupleKeywords', set())):
            dct[keyword] = DefProp(keyword)
        return super(SpecMetaclass, cls).__new__(cls, clsname, bases, dct)

class Spec(object, metaclass=SpecMetaclass):
    keywords = frozenset()
    
    requiredKeywords = frozenset()
    conditionalKeywords = {}
    
    setKeywords = frozenset()
    tupleKeywords = frozenset()
    
    defaultKeyValDict = {}
    
    @property
    def allKeywords(self):
        return (getattr(self, 'keywords', set())
              | getattr(self, 'requiredKeywords', set())
              | getattr(self, 'setKeywords', set())
              | getattr(self, 'tupleKeywords', set()))
    
    def __init__(self, **kwargs):
        self.map = OrderedDict()
        
        # check to see if any of the conditionalKeywords should be added to this spec instance
        self.initConditionalKeywords(**kwargs)
                
        # test if required keywords is a subset of the keyword arguments we actually got
        if not self.requiredKeywords <= kwargs.keys():
            raise
        
        # for all of the setKeywords, make sure that the associated argument is a set
        for key in self.setKeywords:
            if key in kwargs:
                kwargs[key] = Setify(kwargs[key])
        
        # for all of the tupleKeywords, make sure that the associated argument is a tuple
        for key in self.tupleKeywords:
            if key in kwargs:
                kwargs[key] = Tupify(kwargs[key])
            
        # use the genName method to get this instance's name
        name = self.genName(**kwargs)
        if name!=False:
            kwargs['name'] = name
        
        # apply the default key values from defaultKeyValDict
        for key,val in self.defaultKeyValDict.items():
            self[key] = val
        
        # load in all of the values
        for key,val in kwargs.items():
            self[key] = val

# encapsultations of functionality from __init__
    def initConditionalKeywords(self, **kwargs):
        # check to see if any of the conditionalKeywords should be added to this spec instance
        for ckDict in self.conditionalKeywords:
            key = ckDict['checkKeyword']
            if key in kwargs and kwargs[key]==ckDict['equals']:
                self.addKeywords(ckDict['keywords'])
                if 'required' in ckDict and ckDict['required']:
                    self.requiredKeywords = self.requiredKeywords | Setify(ckDict['keywords'])
                else:
                    self.keywords = self.keywords | Setify(ckDict['keywords'])

# magic methods and such
    def __contains__(self, key):
        return key in self.map
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __iter__(self):
        return self.map.__iter__()
    
    def __setitem__(self, key, val):
        if key in self.allKeywords:
            self.map[key] = val
        else:
            raise

# accessors
    def keys(self):
        return self.map.keys()

# other(?)
    def addKeywords(self, keywords):
        for keyword in Setify(keywords):
            self.__setattr__(keyword, KeywordDescriptor(keyword))
    
    def defaultName(self, **kwargs):
        '''
        returns the default name to use for a spec instance if the 'name' keyword is not in args
        '''
        return False
    
    def genName(self, **kwargs):
        '''
        Use the kwargs passed to the __init__ to generate this instance's name. If this function returns false, then just leave the name attribute unset
        '''
        if 'name' in kwargs:
            return kwargs['name']
        else:
            return self.defaultName(**kwargs)