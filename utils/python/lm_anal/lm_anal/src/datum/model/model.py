from lm_anal.src.datum import Datum
from lm_anal.src.datumABC.model import ModelABC
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['Model']

class Model(Datum):
    pass

ModelABC.register(Model)