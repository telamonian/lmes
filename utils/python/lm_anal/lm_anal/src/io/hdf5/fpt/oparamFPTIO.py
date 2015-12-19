from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class PCloudIO(HDF5IO):
    hdf5RootPath = 'FFluxHists'
    hdf5Specs = HDF5Specs(
        HDF5Spec(name='points', fullOnly=True, subKey='Points', type='dataset'))