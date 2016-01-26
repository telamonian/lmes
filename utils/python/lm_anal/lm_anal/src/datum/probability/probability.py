from lm_anal.src.datum.pcloud.pcloud import PCloud
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['FPT']

class FPT(PCloud):
    pointsDtype = [('id', 'int'), ('count', 'int'), ('initial_count', 'int'), ('time', 'float')]
    propertySpecs = DatSpcs(DatSpc(name='points', dtype=pointsDtype, paths=('points'), storageType='numpy', type='array'))

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='fpt', targetName='points')

    # add some field alias specs
    propertySpecs.addFieldAlias(name='id', targetField='id', targetName='points')
    propertySpecs.addFieldAlias(name='count', targetField='count', targetName='points')
    propertySpecs.addFieldAlias(name='inital_count', targetField='initial_count', targetName='points')
    propertySpecs.addFieldAlias(name='time', targetField='time', targetName='points')