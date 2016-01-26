ASCENDING = 0
DESCENDING = 1

import numpy as np

from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs

class TilingsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5IOSpecs(HDF5IOSpec(name='dims', subKey='Edges', type='special'),
                            HDF5IOSpec(name='edges', subKey='Edges', type='dataset'),
                            HDF5IOSpec(name='arrangement', subKey='Edges', type='special'),
                            HDF5IOSpec(name='id', subKey='ID', type='attribute'),
                            HDF5IOSpec(name='order_parameter_id', subKey='OrderParameterID', type='attribute'),
                            HDF5IOSpec(name='rank', subKey='Edges', type='special'),
                            HDF5IOSpec(name='type', subKey='Type', type='special'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
    
    def inputArrangement(self, hdf5Path, hdf5Spec, subCon):
        subCon.setArray(name=hdf5Spec.name, val=np.array([self.getArrangement(subCon.edges)]))
        
    def getArrangement(self, edges):
        # based on first and last edges, infer if this tiling is arranged ASCENDING=0 or DESCENDING=1
        arrangement = ASCENDING if edges[-1]>=edges[0] else DESCENDING
        
        # check the sorting of the edges to make sure our guess is correct
        if arrangement==ASCENDING:
            for edge,nextEdge in zip(edges[:-1],edges[1:]):
                if edge > nextEdge:
                    raise ValueError
        else: # if self.arrangement==DESCENDING:
            for edge,nextEdge in zip(edges[:-1],edges[1:]):
                if nextEdge > edge:
                    raise ValueError
        return arrangement

    def inputDims(self, hdf5Path, hdf5Spec, subCon):
        subCon.setArray(name=hdf5Spec.name, val=np.array(self.file[hdf5Path][hdf5Spec.subKey].shape))

    def inputRank(self, hdf5Path, hdf5Spec, subCon):
        subCon.setScalar(name=hdf5Spec.name, val=len(self.file[hdf5Path][hdf5Spec.subKey].shape))

    def inputType(self, hdf5Path, hdf5Spec, subCon):
        subCon.setType(typeID=self.file[hdf5Path].attrs[hdf5Spec.subKey])
        self.inputAttribute(hdf5Path, hdf5Spec, subCon)