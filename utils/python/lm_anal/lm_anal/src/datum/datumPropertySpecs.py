class DatumPropertySpecs(object):
    def __init__(self, *args):
        self.map = {}
        for arg in args:
            self[arg.name] = arg
        
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val
    