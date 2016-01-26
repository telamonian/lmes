from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['SpeciesFPT']

class SpeciesFPT(FPT):
    # change name of base FPT 'id' field to 'species_id'
    pointsDtype = [('species_id', 'int'), ('count', 'int'), ('initial_count', 'int'), ('time', 'float')]
    propertySpecs = DatSpcs()
    # propertySpecs = DatSpcs(DatSpc(name='points', dtype=pointsDtype, paths=('points'), storageType='numpy', type='array'))

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='speciesFPT', targetName='points')

    # add some field alias specs
    # propertySpecs.addFieldAlias(name='id', targetField='species_id', targetName='points')
    # propertySpecs.addFieldAlias(name='species_id', targetField='species_id', targetName='points')