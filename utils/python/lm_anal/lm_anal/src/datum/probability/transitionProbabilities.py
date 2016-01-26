from lm_anal.src.datum.probability.probabilities import Probabilities
from lm_anal.src.datum.probability.transitionProbability import TransitionProbability
from lm_anal.src.io.hdf5.probability import TransitionProbabilitiesIO

__all__ = ['TransitionProbabilities']

class TransitionProbabilities(Probabilities):
    datumType = TransitionProbability
    hdf5IOType = TransitionProbabilitiesIO
    sfileType = None