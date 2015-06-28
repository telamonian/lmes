from src.datum.datum import Datum
from src.datum.datum import DatumMetaclass

class OParam(Datum, metaclass=DatumMetaclass):
    propertySpecs = {'id':{'dtype':'int', 'paths':('id',), 'storageType':'protoBuf', 'type':'scalar'},
                     'species_ids':{'dtype':'int', 'paths':('species_ids',), 'storageType':'protoBuf', 'type':'array'},
                     'species_coefficients':{'dtype':'float', 'paths':('species_coefficients',), 'storageType':'protoBuf', 'type':'array'},
                     'type':{'dtype':'int', 'paths':('type',), 'storageType':'protoBuf', 'type':'scalar'}}
#                      'dims':{'dtype':'int', 'paths':('dims',), 'storageType':'protoBuf', 'type':'array'},                 
#                      'rank':{'dtype':'int', 'paths':('rank',), 'storageType':'protoBuf', 'type':'scalar'},    
    
    def __init__(self, subcon, full=False):
        super().__init__(full=full)
        self.protoBuf = subcon