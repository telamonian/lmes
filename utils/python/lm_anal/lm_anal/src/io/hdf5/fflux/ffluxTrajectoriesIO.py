import os
import numpy as np

from lm_anal.src.helper import DirectionEnum, LifecycleEnum
from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class FFluxTrajectoriesIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='direction', type='special'),
                          HDF5Spec(fullOnly=False, name='lifecycle', type='special'),
                          HDF5Spec(fullOnly=True, name='count', subKey='Count', type='dataset'),
                          HDF5Spec(fullOnly=True, name='edge_id', subKey='EdgeID', type='dataset'),
                          HDF5Spec(fullOnly=True, name='species_count', subKey='SpeciesCount', type='dataset'),
                          HDF5Spec(fullOnly=True, name='time', subKey='Time', type='dataset'),
                          HDF5Spec(fullOnly=True, name='trajectory_id', subKey='TrajectoryID', type='dataset'))
    
    def inputDirection(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name=hdf5Spec.name, val=DirectionEnum.Value(hdf5Path.split('/')[-2]))

    def inputLifecycle(self, hdf5Path, hdf5Spec, subCon, full):
        subCon.setScalar(name=hdf5Spec.name, val=LifecycleEnum.Value(hdf5Path.split('/')[-1]))
        
    def _keys(self):
        '''
        finds extant FFluxOutput data in an hdf5 file and returns the keys to the Trajectories part of it
        '''
        return [os.path.relpath(lg.name, start='/'+self.hdf5RootPath)  
                for dg in self.file[self.hdf5RootPath].values()         
                for lg in dg.values()                                 
                if ('FORWARD' in dg.name or 'BACKWARD' in dg.name) and ('INITIAL' in lg.name or 'FINAL' in lg.name)]
     
    def _rff(self, container, full, keys):
        '''
        internal rff (read from file) for data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            subCon = container.initDatum(key=key, full=full)
            self.input(full=full, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), subCon=subCon)
            if full:
                subCon.genTrajectoryPhaseMap()