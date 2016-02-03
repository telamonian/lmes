from lm_anal.src.datum.probability.probability import Probability
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['TransitionProbability']

class TransitionProbability(Probability):
    pointsDtype = [('success', 'float'),
                   ('failure', 'float'),
                   ('initial', 'float'),
                   ('count', 'int'),
                   ('probability', 'float'),
                   ('cumulative_probability', 'float')]
    propertySpecs = DatumSpecs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='transition_probability', targetName='points')