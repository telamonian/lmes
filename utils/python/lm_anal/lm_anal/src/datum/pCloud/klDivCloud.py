import numpy as np
import os, sys

from lm_anal.src.datum.pCloud import PCloud
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['KLDivCloud']

class KLDivCloud(PCloud):
    propertySpecs = DatumSpecs()
    
    # add some alias specs
    propertySpecs.addAlias(name='kl_div', targetName='points')
    
    def __init__(self, full=False):
        super().__init__(full=full) 
        self.labels = np.array(['kl_div'])