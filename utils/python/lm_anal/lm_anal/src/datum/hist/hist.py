from lm_anal.src.datum.datum import Datum
from lm_anal.src.datum.datum import DatumMetaclass

class Hist(Datum, metaclass=DatumMetaclass):
    propertySpecs = {'edges':{'dtype':'float', 'storageType':'numpy', 'type':'array'},
                     'h':{'dtype':'float', 'storageType':'numpy', 'type':'array'}}
    
    def __init__(self, full=False):
        super().__init__(full=full)
    
    @property
    def dims(self):
        return self.h.shape
    @property
    def rank(self):
        return len(self.h.shape)