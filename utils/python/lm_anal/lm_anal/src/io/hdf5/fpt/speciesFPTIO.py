from lm_anal.src.io.hdf5 import HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.pCloud.pCloudIO import PCloudIO

class SpeciesFPTIO(PCloudIO):
    hdf5RootPath = 'SpeciesFPTs'
    hdf5Specs = HDF5Specs(
        HDF5Spec(name='points', fullOnly=True, subKey='Points', type='dataset'))