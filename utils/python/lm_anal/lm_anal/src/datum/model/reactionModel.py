import numpy as np

from lm_anal.src.datum.model.model import Model
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['ReactionModel']

class ReactionModel(Model):
    propertySpecs = DatSpcs(
        DatSpc(name='dependency_matrix', dtype=np.uint, paths=('dependency_matrix'), storageType='numpy', type='array'),
        DatSpc(name='initial_species_counts', dtype=np.uint, paths=('initial_species_counts'), storageType='numpy', type='array'),
        DatSpc(name='initial_species_counts_backward', dtype=np.uint, paths=('initial_species_counts_backward'), storageType='numpy', type='array'),
        DatSpc(name='reaction_rate_constants', dtype=np.float64, paths=('reaction_rate_constants'), storageType='numpy', type='array'),
        DatSpc(name='reaction_types', dtype=np.uint, paths=('reaction_types'), storageType='numpy', type='array'),
        DatSpc(name='stoichiometric_matrix', dtype=np.int, paths=('stoichiometric_matrix'), storageType='numpy', type='array')
    )