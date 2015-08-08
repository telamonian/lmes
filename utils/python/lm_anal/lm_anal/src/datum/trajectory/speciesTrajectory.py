from lm_anal.src.datum.trajectory.trajectory import Trajectory
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['SpeciesTrajectory']

class SpeciesTrajectory(Trajectory):
    propertySpecs = DatumSpecs(DatumSpec(name='number_species', dtype='int', paths=('cme_state','species_counts','number_species'), storageType='protobuf', type='scalar'),
                               DatumSpec(name='species_count', dtype='int', storageType='numpy', type='array'),
                               DatumSpec(name='time', dtype='float', storageType='numpy', type='array'))
        