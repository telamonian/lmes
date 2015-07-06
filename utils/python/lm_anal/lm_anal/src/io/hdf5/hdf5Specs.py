from collections import OrderedDict

class HDF5Specs(object):
    def __init__(self, *args):
        self.map = OrderedDict()
        for arg in args:
            self[arg.name] = arg
        
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val
        
    def __iter__(self):
        return self.map.values().__iter__()
    