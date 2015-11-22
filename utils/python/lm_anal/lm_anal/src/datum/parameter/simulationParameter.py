import numpy as np

from lm.io.SimulationParameters_pb2 import SimulationParameters as SimulationParametersBuf
from lm_anal.src.datum import Datum, DatumMetaclass, DatumPropertySpec, DatumPropertySpecs

class SimulationParameter(Datum, metaclass=DatumMetaclass):
    propertySpecs = DatumPropertySpecs(DatumPropertySpec(dtype='str', name='key', paths=('key',), storageType='protobuf', type='array'),
                                       DatumPropertySpec(dtype='str', name='value', paths=('value',), storageType='protobuf', type='array'))
    
    def __init__(self, full=False):
        super().__init__(full=full)
        self.protobuf = SimulationParametersBuf()
        
    def __getitem__(self, key):
        for i,spKey in enumerate(self.key):
            if spKey==key:
                return self.value[i]
        raise AttributeError