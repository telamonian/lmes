import os
import numpy as np

from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class FFluxOutputsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='number_species', subKey='NumberSpecies', type='attribute'),
                          HDF5Spec(fullOnly=False, name='number_tiles', subKey='NumberTiles', type='attribute'),
                          HDF5Spec(fullOnly=False, name='tiling_id', subKey='TilingID', type='attribute'),
                          HDF5Spec(fullOnly=False, name='type', subKey='Type', type='embedded'))
#                  HDF5Spec(fullOnly=False, name='dims', subKey='Edges', type='special'),
#                  HDF5Spec(fullOnly=False, name='rank', subKey='Edges', type='special'),
    
    def inputType(self, hdf5Path, hdf5Spec, subCon):
        subCon.setType(typeID=self.file[hdf5Path].attrs[hdf5Spec.subKey])
        self.inputAttribute(hdf5Path, hdf5Spec, subCon)
    
#     def __init__(self, fPath):
#         super().__init__(fPath)

#     def inputRank(self, hdf5Path, hdf5Spec, subCon):
#         subCon.setScalar(name=hdf5Spec.name, val=len(self.file[hdf5Path][hdf5Spec.subKey].shape))
# 
#     def inputDims(self, hdf5Path, hdf5Spec, subCon):
#         subCon.setArray(name=hdf5Spec.name, val=np.array(self.file[hdf5Path][hdf5Spec.subKey].shape))