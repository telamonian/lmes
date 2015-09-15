from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class KLDivCloudIO(HDF5IO):
    hdf5RootPath = 'KLDivClouds'
    hdf5Specs = HDF5Specs(HDF5Spec(name='labels', fullOnly=False, subKey='Labels', type='dataset'),
                          HDF5Spec(name='klDiv', fullOnly=True, subKey='KLDiv', type='dataset'))