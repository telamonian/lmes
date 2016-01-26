import os
import numpy as np

# from lm_anal.src.datum.fflux import FFluxBasins, FFluxFinals, FFluxTrajectories
from lm_anal.src.helper import LazyClass
from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.fflux import FFluxBasinsIO, FFluxFinalsIO, FFluxTrajectoriesIO

lzFFluxBasins = LazyClass(modName='lm_anal.src.datum.fflux', clsName='FFluxBasins')
lzFFluxFinals = LazyClass(modName='lm_anal.src.datum.fflux', clsName='FFluxFinals')
lzFFluxTrajectories = LazyClass(modName='lm_anal.src.datum.fflux', clsName='FFluxTrajectories')

class FFluxOutputsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='number_species', subKey='NumberSpecies', type='attribute'),
                          HDF5Spec(fullOnly=False, name='number_tiles', subKey='NumberTiles', type='attribute'),
                          HDF5Spec(fullOnly=False, name='tiling_id', subKey='TilingID', type='attribute'),
                          HDF5Spec(DataType=lzFFluxBasins, IOType=FFluxBasinsIO, fullOnly=False, name='basins', type='embedded'),
                          HDF5Spec(DataType=lzFFluxFinals, IOType=FFluxFinalsIO, fullOnly=False, name='finals', type='specialSubData'),
                          HDF5Spec(DataType=lzFFluxTrajectories, IOType=FFluxTrajectoriesIO, fullOnly=False, name='trajectories', type='embedded'))
    
    def inputFinals(self, excludedFields, hdf5Path, hdf5Spec, o, subCon):
        subData = self.inputEmbedded(excludedFields=excludedFields, hdf5Path=hdf5Path, hdf5Spec=hdf5Spec, o=o, subCon=subCon)
        subCon.final = subData[0]
        
    def _keys(self):
        '''
        finds extant FFluxOutput data in an hdf5 file and returns the keys to it
        '''
        return [os.path.relpath(ffog.name, start='/'+self.hdf5RootPath)  
                for tg in self.file[self.hdf5RootPath].values()         
                for ffog in tg.values()                                 
                if 'FFluxOutput' in ffog.name]
    
    def _rff(self, container, excludedFields, keys, o):
        '''
        internal rff (read from file) for FFluxOutput data stored in hdf5 files
        '''
        if keys==None:
            keys = self.keys()
    
        for key in keys:
            dataKey = int(os.path.split(key)[0])
            subCon = container.initDatum(key=dataKey, o=o)
            try:
                self.input(excludedFields=excludedFields, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), o=o, subCon=subCon)
            except KeyError:
                del container[dataKey]
                key = '%07d' % key
                subCon = container.initDatum(key=dataKey, o=o)
                self.input(excludedFields=excludedFields, hdf5Path=os.path.join(self.hdf5RootPath, str(key)), o=o, subCon=subCon)