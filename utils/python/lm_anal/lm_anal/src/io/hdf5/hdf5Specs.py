from collections import OrderedDict
from copy import deepcopy

__all__ = ['HDF5Specs']

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
    
    def combine(self, *others):
        newSpecs = deepcopy(self)
        newSpecs.update(others)
        return newSpecs
    
    def update(self, *others):
        for other in others:
            self.map.update(other.map)