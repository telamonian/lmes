import numpy as np

from .oparam import OParam

class OParamLinear(OParam):
    def calc(self, speciesCounts):
        return speciesCounts[:,:self.SpeciesCoefficients.shape[0]].dot(self.SpeciesCoefficients.T)