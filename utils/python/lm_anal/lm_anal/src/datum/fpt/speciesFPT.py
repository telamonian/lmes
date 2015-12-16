from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['SpeciesFPT']

class SpeciesFPT(FPT):
    # change name of base FPT 'id' field to 'species_id'
    propertySpecs = DatSpcs(DatSpc(name='points', dtype=[('species_id', 'int'), ('count', 'int'), ('time', 'float')], paths=('points'), storageType='numpy', type='array'))

    # add some field alias specs
    propertySpecs.addFieldAlias(name='id', targetField='species_id', targetName='points')