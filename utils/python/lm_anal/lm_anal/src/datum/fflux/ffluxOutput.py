from lm_anal.src.datum import Datum

class FFluxOutput(Datum):
    propertySpecs = {'id':{'dtype':'int', 'paths':('id',), 'storageType':'protoBuf', 'type':'scalar'},
                     'species_ids':{'dtype':'int', 'paths':('species_ids',), 'storageType':'numpy', 'type':'array'},
                     'species_coefficients':{'dtype':'float', 'paths':('species_coefficients',), 'storageType':'numpy', 'type':'array'},
                     'type':{'dtype':'int', 'paths':('type',), 'storageType':'protoBuf', 'type':'scalar'}}
#                      'dims':{'dtype':'int', 'paths':('dims',), 'storageType':'protoBuf', 'type':'array'},                 
#                      'rank':{'dtype':'int', 'paths':('rank',), 'storageType':'protoBuf', 'type':'scalar'},

    oparamSubtypeDict = {}
    
    @classmethod
    def registerSubtype(cls, typeID):
        cls.oparamSubtypeDict[typeID] = cls
    
    @property
    def name(self):
        try:
            return self._name
        except AttributeError:
            return self.id
    @name.setter
    def name(self, val):
        self._name = val
    
    def __init__(self, subcon, full=False):
        super().__init__(full=full)
        self.protoBuf = subcon
        
    def init(self):
        pass
    
    def calc(self):
        pass
    
    def combine(self, others):
        pass
    
    def setType(self, typeID):
        self.__class__ = self.oparamSubtypeDict[typeID]
        self.init()