from lm_anal.python_protobuf.lm.io.SimulationParameters_pb2 import SimulationParameters as SimulationParametersBuf
from lm_anal.src.datum import Datum, DatumMetaclass, DatumPropertySpec, DatumPropertySpecs


class SimulationParameter(Datum, metaclass=DatumMetaclass):
    propertySpecs = DatumPropertySpecs(DatumPropertySpec(dtype='str', name='key', paths=('key',), storageType='protobuf', type='array'),
                                       DatumPropertySpec(dtype='str', name='value', paths=('value',), storageType='protobuf', type='array'))
    
    def __init__(self, full=False):
        super().__init__(full=full)
        self.protobuf = SimulationParametersBuf()