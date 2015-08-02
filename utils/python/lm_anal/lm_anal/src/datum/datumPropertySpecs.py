from collections import OrderedDict

__all__ = ['DatumPropertySpecs']

class DatumPropertySpecs(object):
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
        return self.map.__iter__()
    
    def items(self):
        return self.map.items()