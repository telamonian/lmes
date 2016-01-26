import os
import numpy as np
import re

from lm_anal.src.helper import DirectionEnum
from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs

class FFluxFinalsIO(HDF5IO):
    hdf5RootPath = 'Tilings'
    hdf5Specs = HDF5Specs(HDF5Spec(fullOnly=False, name='probability_i', subKey='ProbabilityI/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='normalized_probability_i', subKey='ProbabilityI/TileVals', type='dataset'),
                          HDF5Spec(fullOnly=False, name='probability_i_weight', subKey='ProbabilityIWeight', type='attribute'),
                          HDF5Spec(fullOnly=False, name='switching_rate_constants', type='special'))
                          
    def inputSwitchingRateConstants(self, hdf5Path, hdf5Spec, subCon):
        sRCs = []
        for key,val in self.file[hdf5Path].attrs.items():
            sRCRE = re.search('SwitchingRateConstant_FromBasin(\d+)', key)
            if sRCRE:
                sRCs.append((int(sRCRE.group(1)), float(val)))
        sRCs = np.array(sorted(sRCs))
        subCon.setArray(name=hdf5Spec.name, val=sRCs[:,1])

    def _rff(self, container, excludedFields, keys, o):
        '''
        internal rff (read from file) for data stored in hdf5 files
        '''
        subCon = container.initDatum(key=0)
        self.input(excludedFields=excludedFields, hdf5Path=self.hdf5RootPath, subCon=subCon)