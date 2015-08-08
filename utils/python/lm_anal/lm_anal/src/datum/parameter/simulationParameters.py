import os,sys

from lm_anal.src.datum.data import Data
from lm_anal.src.datum.parameter import SimulationParameter

class SimulationParameters(Data):
    datumType = SimulationParameter
    
    def __init__(self):
        super().__init__()

    def get(self, key):
        return self.peek()[key]

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            self.__setattr__('simulationParameter', self.map[key])
            return self.map[key]