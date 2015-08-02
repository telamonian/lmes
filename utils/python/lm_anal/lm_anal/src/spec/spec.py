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
 
class SpecMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        for keyword in (dct.get('keywords', set()) |
                        dct.get('requiredKeywords', set()) |
                        dct.get('setKeywords', set()) |
                        dct.get('tupleKeywords', set())):
            dct[keyword] = DefProp(keyword)
        return super(SpecMetaclass, cls).__new__(cls, clsname, bases, dct)

class Spec(object, metaclass=SpecMetaclass):
    keywords = set()
    
    requiredKeywords = set()
    setKeywords = set()
    tupleKeywords = set()
    
    def __init__(self, **kwargs):
        self.map = OrderedDict()
        
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
        
        for key,val in kwargs.items():
            self[key] = val
    
    def __contains__(self, key):
        return key in self.map
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        if key in self.keywords:
            self.map[key] = val
        else:
            raise AttributeError
    
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