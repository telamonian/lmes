import numpy as np
import os, sys

from lm_anal.src.datum.pcloud import PCloud
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['KLDivCloud']

class KLDivCloud(PCloud):
    propertySpecs = DatSpcs()

    # add some alias specs
    propertySpecs.addAlias(name='kl_div', targetName='points')