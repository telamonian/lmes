import os, sys

from lm_anal.src.datum import Datum
from lm_anal.src.datumABC.trajectory import PCloudABC
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['PCloud']

class PCloud(Datum):
    propertySpecs = DatumSpecs(DatumSpec(name='labels', dtype='string', paths=('points'), storageType='numpy', type='array'),
                               DatumSpec(name='points', dtype='float', paths=('points'), storageType='numpy', type='array'))
        
PCloudABC.register(PCloud)