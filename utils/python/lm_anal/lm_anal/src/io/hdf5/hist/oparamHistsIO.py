from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.hist.histsIO import HistsIO

class OParamHistsIO(HistsIO):
    hdf5RootPath = 'OParamHists'