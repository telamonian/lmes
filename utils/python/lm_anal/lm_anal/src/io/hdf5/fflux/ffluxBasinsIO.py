import os
import numpy as np

from lm_anal.src.helper import DirectionEnum
from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class FFluxBasinsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='direction', type='special'),
                          HDF5Spec(fullOnly=False, name='flux_out_of_tile_zero', subKey='FluxOutOfTileZero', type='attribute'),
                          HDF5Spec(fullOnly=False, name='probability_i_to_i_plus_one', subKey='ProbabilityIToIPlusOne/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='probability_one_to_i_plus_one', subKey='ProbabilityOneToIPlusOne/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='switching_rate_constant', subKey='SwitchingRateConstant', type='attribute'),
                          HDF5Spec(fullOnly=False, name='this_basin_last_visited_probability', subKey='ThisBasinLastVisitedProbability', type='attribute'),
#                           HDF5Spec(fullOnly=False, name='probability_i', subKey='ProbabilityI/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='normalized_probability_i', subKey='ProbabilityI/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='probability_i_weight', subKey='ProbabilityIWeight', type='attribute'))
    
    def inputDirection(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name=hdf5Spec.name, val=DirectionEnum.Value(hdf5Path.split('/')[-1]))
        
    def _keys(self):
        '''
        finds extant FFluxOutput data in an hdf5 file and returns the keys to the Trajectories part of it
        '''
        return [os.path.relpath(dg.name, start='/'+self.hdf5RootPath)  
                for dg in self.file[self.hdf5RootPath].values()                              
                if 'FORWARD' in dg.name or 'BACKWARD' in dg.name]
     
    def _rff(self, container, full, keys):
        '''
        internal rff (read from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            subCon = container.initDatum(key=key, full=full)
            self.input(full=full, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), subCon=subCon)
