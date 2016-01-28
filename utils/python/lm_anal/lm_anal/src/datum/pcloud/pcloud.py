import numpy as np

from lm_anal.src.datum.datum import Datum, DatumMetaclass
from lm_anal.src.datumABC.pcloud import PCloudABC
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['PCloud']

class PCloudMetaclass(DatumMetaclass):
    def __new__(cls, clsname, bases, dct):
        if 'pointsDtype' in dct and dct['pointsDtype'] is not None:
            dtype = np.dtype(dct['pointsDtype'])
            if 'propertySpecs' not in dct or dct['propertySpecs'] is None:
                dct['propertySpecs'] = DatSpcs()
            dct['propertySpecs'].addSpecList([DatSpc(name='points', dtype=dtype, paths=('points'), storageType='numpy', type='array')])
            if dtype.names is not None:
                for name in dtype.names:
                    dct['propertySpecs'].addFieldAlias(name=name, targetField=name, targetName='points')
        return super().__new__(cls, clsname, bases, dct)

class PCloud(Datum, metaclass=PCloudMetaclass):
    pointsDtype = 'float'

PCloudABC.register(PCloud)