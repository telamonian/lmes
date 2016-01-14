from lm_anal.src.io.hdf5 import HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.pcloud.pcloudIO import PCloudIO

class FPTIO(PCloudIO):
    hdf5RootPath = 'FPTs'