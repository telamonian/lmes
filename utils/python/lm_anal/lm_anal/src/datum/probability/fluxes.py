from lm_anal.src.datum.probability.probabilities import Probabilities
from lm_anal.src.datum.probability.flux import Flux
from lm_anal.src.io.hdf5.probability import FluxIO

__all__ = ['TransitionProbabilities']

class TransitionProbabilities(Probabilities):
    datumType = Flux
    hdf5IOType = FluxIO
    sfileType = None