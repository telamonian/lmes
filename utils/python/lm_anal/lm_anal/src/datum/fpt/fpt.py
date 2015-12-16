import numpy as np
import os, sys

from lm_anal.src.datum.pCloud import PCloud
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['FPT']

class FPT(PCloud):
    propertySpecs = DatSpcs(DatSpc(name='points', dtype=[('id', 'int'), ('count', 'int'), ('time', 'float')], paths=('points'), storageType='numpy', type='array'))

    # add some field alias specs
    propertySpecs.addFieldAlias(name='id', targetField='id', targetName='points')
    propertySpecs.addFieldAlias(name='count', targetField='count', targetName='points')
    propertySpecs.addFieldAlias(name='time', targetField='time', targetName='points')