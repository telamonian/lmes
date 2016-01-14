import os
import numpy as np

from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class SimulationParametersIO(HDF5IO):
    hdf5RootPath = 'Parameters'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='key', type='special'),
                          HDF5Spec(fullOnly=False, name='value', type='special'))
    
    def __init__(self, fPath):
        super().__init__(fPath)
    
    def _has(self):
        if self.hdf5RootPath in self.file:
            return True
        else:
            return False
        
    def inputKey(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setArray(name=hdf5Spec.name, val=np.array(list(self.file[hdf5Path].attrs.keys())))

    def inputValue(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setArray(name=hdf5Spec.name, val=np.array(list(self.file[hdf5Path].attrs.values())))

    def _keys(self):
        '''
        returns ''. That's the key!
        '''
        return ['']
    
    def _rff(self, container, full, keys):
        '''
        internal rff (read from file) for FFluxOutput data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            subCon = container.initDatum(key='Parameters', full=full)
            self.input(full=full, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), subCon=subCon)
