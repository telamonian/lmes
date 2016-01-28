from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class HistsIO(HDF5IO):
    hdf5RootPath = 'Hists'
    hdf5Specs = HDF5IOSpecs(HDF5IOSpec(name='h', fullOnly=False, subKey='h', type='histogram'))
                          # HDF5IOSpec(name='tilings', fullOnly=False, IOType=TilingsIO, subKey='tilings', type='subData')