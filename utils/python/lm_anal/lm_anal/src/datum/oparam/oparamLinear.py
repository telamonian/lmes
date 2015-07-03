from copy import deepcopy
import numpy as np

from lm_anal.src.datum.oparam import OParam

class OParamLinear(OParam):
    @property
    def numberOrderParameters(self):
        return self.multiplier.shape[1]
    
    def init(self):
        self.multiplier = np.zeros((self.species_ids.max() - self.species_ids.min() + 1))
        for i,coeff in zip((sid - self.species_ids.min() for sid in self.species_ids), self.species_coefficients):
            self.multiplier[i] = coeff
        self.multiplier = self.multiplier.reshape((-1, 1))
    
    def calc(self, speciesCounts):
        return speciesCounts[:,self.species_ids.min():self.species_ids.max()+1].dot(self.multiplier)
    
    def combine(self, others):
        newOP = deepcopy(self)
#         multipliers = [newOP.multiplier]
        species_coefficients = [newOP.species_coefficients]
        species_ids = [newOP.species_ids]
        maxSpecID = np.max(newOP.species_ids)
        minSpecID = np.min(newOP.species_ids)
        for op in others:
#             multipliers.append(op.multiplier)
            species_coefficients.append(op.species_coefficients)
            species_ids.append(op.species_ids)
            maxSpecID = np.max([maxSpecID, np.max(op.species_ids)])
            minSpecID = np.min([minSpecID, np.min(op.species_ids)])
        newOP.multiplier = np.zeros((maxSpecID - minSpecID + 1, len(species_ids)))
        for j,(specIDs,specCoeffs) in enumerate(zip(species_ids, species_coefficients)):
            for i,coeff in zip((sid - minSpecID for sid in specIDs), specCoeffs):
                newOP.multiplier[i,j] = coeff
        return newOP
    
OParamLinear.registerSubtype(typeID=0)