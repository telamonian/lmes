from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.tiling.tilingsIO import TilingsIO

class HistsIO(HDF5IO):
    hdf5RootPath = 'Hists'
    hdf5Specs = HDF5Specs(HDF5Spec(name='h', fullOnly=False, subKey='h', type='histogram'))
                          # HDF5Spec(name='tilings', fullOnly=False, IOType=TilingsIO, subKey='tilings', type='subData')