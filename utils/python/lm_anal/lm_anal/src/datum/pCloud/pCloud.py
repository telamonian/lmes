import os, sys

from lm_anal.src.datum import Datum
from lm_anal.src.datumABC.trajectory import PCloudABC
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['PCloud']

class PCloud(Datum):
    propertySpecs = DatSpcs(DatSpc(name='points', dtype='float', paths=('points'), storageType='numpy', type='array'))
        
PCloudABC.register(PCloud)