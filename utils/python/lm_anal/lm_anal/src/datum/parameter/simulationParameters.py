import os,sys

from lm_anal.src.datum.dataSingleton import DataSingleton
from lm_anal.src.datum.parameter import SimulationParameter
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO

class SimulationParameters(DataSingleton):
    singletonKey = 'Parameters'

    datumType = SimulationParameter
    hdf5IOType = SimulationParametersIO
    sfileType = None

    # def get(self, key):
    #     return self.peek()[key]

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            self.__setattr__('Parameters', self.map[key])
            return self.map[key]