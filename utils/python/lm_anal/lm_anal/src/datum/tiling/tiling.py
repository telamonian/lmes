ASCENDING = 0
DESCENDING = 1

from lm_anal.src.datum.datumSubtypable import DatumSubtypable   #, DatumMetaclass
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

class Tiling(DatumSubtypable):    #, metaclass=DatumMetaclass):
    subtypeID = None

    propertySpecs = DatSpcs(
        DatSpc(name='arrangement', dtype='int', paths=('arrangement',), storageType='numpy', type='array'),
        DatSpc(name='dims', dtype='int', paths=('dims',), storageType='numpy', type='array'),
        DatSpc(name='edges', dtype='float', paths=('edges',), storageType='numpy', type='array'),
        DatSpc(name='id', dtype='int', paths=('id',), storageType='default', type='scalar'),
        DatSpc(name='order_parameter_id', dtype='int', paths=('order_parameter_id',), storageType='default', type='scalar'),
        DatSpc(name='rank', dtype='int', paths=('rank',), storageType='default', type='scalar'),
        DatSpc(name='type', dtype='int', paths=('type',), storageType='default', type='scalar'))

    @property
    def name(self):
        try:
            return self._name
        except AttributeError:
            return self.id
    @name.setter
    def name(self, val):
        self._name = val
