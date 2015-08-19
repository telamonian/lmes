import os,sys

from lm_anal.src.datum.data import Data
from lm_anal.src.datum.parameter import SimulationParameter
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO

class SimulationParameters(Data):
    datumType = SimulationParameter
    Hdf5IOType = SimulationParametersIO
    SFileType = None

    def get(self, key):
        return self.peek()[key]

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            self.__setattr__('simulationParameter', self.map[key])
            return self.map[key]