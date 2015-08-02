from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxBasin

__all__ = ['FFluxBasins']

class FFluxBasins(Data):
    datumType = FFluxBasin

    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            if self.protobuf!=None:
                subBuf = self.protobuf.basin_outputs.add()
            else:
                subBuf = None
            self.map[key] = self.datumType(subBuf=subBuf, **kwargs)
            return self.map[key]