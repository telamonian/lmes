from types import MethodType

def DefProp(name):
    @property
    def prop(self):
        return self[name]
    @prop.setter
    def prop(self, val):
        self[name] = val
    return prop

class DatumPropertySpecMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        for keyword in dct['keywords']:
            dct[keyword] = DefProp(keyword)
        return super(DatumPropertySpecMetaclass, cls).__new__(cls, clsname, bases, dct)

class DatumPropertySpec(object, metaclass=DatumPropertySpecMetaclass):
    keywords = {'dtype', 'name', 'paths', 'storageType', 'targetName', 'type'}
    
    def __init__(self, name, **kwargs):
        self.map = {}
        self.name = name
        for key,val in kwargs.items():
            self.__setattr__(key, val)
        
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        if key in self.keywords:
            self.map[key] = val
        else:
            raise AttributeError