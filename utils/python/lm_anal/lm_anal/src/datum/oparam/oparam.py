from lm_anal.src.datum import Datum
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

class OParam(Datum):
    propertySpecs = DatSpcs(
         DatSpc(name='id', dtype='int', paths=('id',), storageType='default', type='scalar'),
         DatSpc(name='species_ids', dtype='int', paths=('species_ids',), storageType='numpy', type='array'),
         DatSpc(name='species_coefficients', dtype='float', paths=('species_coefficients',), storageType='numpy', type='array'),
         DatSpc(name='type', dtype='int', paths=('type',), storageType='default', type='scalar'),
         # DatSpc(name='dims', dtype='int', paths=('dims',), storageType='protobuf', type='array'),
         # DatSpc(name='rank', dtype='int', paths=('rank',), storageType='protobuf', type='scalar'),
    )
    subtypeDict = {}
    
    @classmethod
    def registerSubtype(cls, typeID):
        cls.subtypeDict[typeID] = cls
    
    @property
    def name(self):
        try:
            return self._name
        except AttributeError:
            return self.id
    @name.setter
    def name(self, val):
        self._name = val
    
    # def __init__(self, subcon, full=False):
    #     super().__init__(full=full)
    #     self.protobuf = subcon
        
    def init(self):
        pass
    
    def calc(self):
        pass
    
    def combine(self, others):
        pass
    
    def setType(self, typeID):
        self.__class__ = self.subtypeDict[typeID]
        self.init()