import h5py

class DataMetaclass(object):
    def __new__(cls, clsname, bases, dct):
        pass

class Data(object):
    datumType = None
    
    def __init__(self):
        self.protobuf = None
        self.map = {}
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            return self.map[key]