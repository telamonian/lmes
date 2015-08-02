from collections import OrderedDict

__all__ = ['Specs']

class Specs(object):
    def __init__(self, *args):
        self.counter = 0
        self.map = OrderedDict()
        for arg in args:
            self[self.genKey(arg, args)] = arg
    
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
    
    def items(self):
        return self.map.items()
    
    def defaultKey(self, arg, args):
        key = self.counter
        self.counter+=1
        return key
    
    def genKey(self, arg, args):
        if 'name' in arg:
            return arg['name']
        else:
            return self.defaultKey(arg, args)
    
    def size(self):
        return len(self.map)
    
    def values(self):
        return self.map.values()