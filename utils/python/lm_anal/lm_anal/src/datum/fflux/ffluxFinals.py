from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxFinal

class FFluxFinals(Data):
    datumType = FFluxFinal

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            if self.protobuf!=None:
                subBuf = self.protobuf.final_output
            else:
                subBuf = None
            self.map[key] = self.datumType(subBuf=subBuf, **kwargs)
            self.__setattr__('ffluxFinal', self.map[key])
            return self.map[key]