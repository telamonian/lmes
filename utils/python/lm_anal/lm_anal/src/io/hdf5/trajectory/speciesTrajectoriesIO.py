import os

from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class SpeciesTrajectoriesIO(HDF5IO):
    hdf5RootPath = 'Simulations'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='number_entries', subKey='SpeciesCounts', type='special'),
                          HDF5Spec(fullOnly=False, name='number_species', subKey='SpeciesCounts', type='special'),
                          HDF5Spec(fullOnly=False, name='trajectory_id', subKey='ID', type='special'),
                          HDF5Spec(fullOnly=True, name='species_count', subKey='SpeciesCounts', type='dataset'),
                          HDF5Spec(fullOnly=True, name='time', subKey='SpeciesCountTimes', type='dataset'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
    
# special input methods
    def inputNumberEntries(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name=hdf5Spec.name, val=self.file[hdf5Path][hdf5Spec.subKey].shape[0])

    def inputNumberSpecies(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name='number_species', val=self.file[hdf5Path][hdf5Spec.subKey].shape[1])

    def inputTrajectoryId(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name='trajectory_id', val=int(os.path.split(hdf5Path)[-1]))

# special output methods
    def outputNumberEntries(self, hdf5Path, hdf5Spec, subCon):
        pass
    
    def outputNumberSpecies(self, hdf5Path, hdf5Spec, subCon):
        pass

    def outputTrajectoryID(self, hdf5Path, hdf5Spec, subCon):
        self.outputAttribute(hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, subCon=subCon)