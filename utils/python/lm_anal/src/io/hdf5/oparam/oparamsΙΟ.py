import os

from src.io.hdf5.hdf5IO import HDF5IO, HDF5Spec

class OParamsIO(HDF5IO):
    hdf5RootPath = 'OrderParameters'
    hdf5Specs = (HDF5Spec(fullOnly=False, name='id', subKey='ID', type='attribute'),
                 HDF5Spec(fullOnly=False, name='type', subKey='Type', type='attribute'),
                 HDF5Spec(fullOnly=False, name='species_ids', subKey='SpeciesIDs', type='array'),
                 HDF5Spec(fullOnly=False, name='species_coefficients', subKey='SpeciesCoefficients', type='array'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
        