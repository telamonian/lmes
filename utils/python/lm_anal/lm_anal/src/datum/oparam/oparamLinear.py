import numpy as np

from lm_anal.src.datum.oparam.oparam import OParam

class OParamLinear(OParam):
    @property
    def numberOrderParameters(self):
        return self.multiplier.shape[1]
    
    def Init(self):
        self.multiplier = np.zeros((self.species_ids.max() - self.species_ids.min() + 1))
        for i,coeff in zip((sid - self.species_ids.min() for sid in self.species_ids), self.species_coefficients):
            self.multiplier[i] = coeff
        self.multiplier = self.multiplier.reshape((-1, 1))
    
    def calc(self, speciesCounts):
        return speciesCounts[:,self.species_ids.min():self.species_ids.max()+1].dot(self.multiplier)
    
OParamLinear.registerSubtype(typeID=0)