from lm_anal.src.datum.pcloud import PClouds
from lm_anal.src.datum.probability.probability import Probability
from lm_anal.src.io.hdf5.probability import ProbabilitiesIO

__all__ = ['Probabilities']

class Probabilities(PClouds):
    datumType = Probability
    hdf5IOType = ProbabilitiesIO
    sfileType = None