import os

from src.io.hdf5.hdf5IO import HDF5IO, HDF5Spec

class TilingsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = (HDF5Spec(fullOnly=False, name='number_entries', subKey='SpeciesCounts', type='special'),
                 HDF5Spec(fullOnly=False, name='number_species', subKey='SpeciesCounts', type='special'),
                 HDF5Spec(fullOnly=False, name='trajectory_id', subKey='trajectory_id', type='special'),
                 HDF5Spec(fullOnly=False, name='species_count', subKey='SpeciesCounts', type='array'),
                 HDF5Spec(fullOnly=False, name='time', subKey='SpeciesCountTimes', type='array'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
        