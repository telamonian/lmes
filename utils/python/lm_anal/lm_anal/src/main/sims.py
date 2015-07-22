from lm_anal.src.main import Sim

class SimsMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimsMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sims(object):
    def __init__(self, fPath=None):
        self.map = {}
        
        if fPath is not None:
            self.initSim(0, fPath=fPath)
    
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.items().__iter__()

    def initSim(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = Sim(**kwargs)
            return self.map[key]
        
    def map(self, recipeName, **kwargs):
        self.__getattribute__('%sMap' % recipeName)(**kwargs)
        
    def OParamHistsMap(self, tilingIDs, **kwargs):
        for sim in self:
            sim.map('OParamHists', tilingIDs=tilingIDs)