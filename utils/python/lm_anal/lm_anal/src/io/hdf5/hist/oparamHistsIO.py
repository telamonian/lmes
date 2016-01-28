from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs
from lm_anal.src.io.hdf5.hist.histsIO import HistsIO

class OParamHistsIO(HistsIO):
    hdf5RootPath = 'OParamHists'