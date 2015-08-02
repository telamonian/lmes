import h5py

__all__ = ['DataMetaclass', 'Data']

class DataMetaclass(object):
    def __new__(cls, clsname, bases, dct):
        return super(DataMetaclass, cls).__new__(cls, clsname, bases, dct)

class Data(object):
    datumType = None

# initializers
    def __init__(self, protobuf=None):
        self.protobuf = protobuf
        self.map = {}

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            return self.map[key]
    
# magic methods and the like
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.items().__iter__()

    def keyIter(self, keys=None):
        # the keys keyword is here mostly for symmetry with valIter
        if keys==None:
            return self.map.keys().__iter__()
        else:
            for key in keys:
                yield key

    def valIter(self, keys=None):
        if keys==None:
            return self.map.values().__iter__()
        else:
            for key in keys:
                yield self[key]

# accessors
    def keys(self):
        return self.map.keys()
    
    def items(self):
        return self.map.items()
    
    def vals(self):
        return self.map.vals()
    