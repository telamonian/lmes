from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class PCloudsIO(HDF5IO):
    hdf5RootPath = 'FFluxHists'
    hdf5Specs = HDF5IOSpecs(
        HDF5IOSpec(name='points', fullOnly=True, subKey='Points', type='dataset'))