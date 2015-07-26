from collections import OrderedDict

class Specs(object):
    def __init__(self, *args):
        self.counter = 0
        self.map = OrderedDict()
        for arg in args:
            self[self.genKey(arg, args)] = arg
        
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
    
    def defaultKey(self, args, arg):
        key = self.counter
        self.counter+=1
        return key
    
    def genKey(self, args, arg):
        if 'name' in arg:
            return arg['name']
        else:
            return self.defaultName(arg)
    
    def size(self):
        return len(self.map)