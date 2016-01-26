from lm_anal.src.datum.probability.probability import Probability
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['TransitionProbability']

class TransitionProbability(Probability):
    pointsDtype = [('start', 'float'), ('success', 'float'), ('failure', 'float'), ('probability', 'float')]
    propertySpecs = DatSpcs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='transitionProbability', targetName='points')