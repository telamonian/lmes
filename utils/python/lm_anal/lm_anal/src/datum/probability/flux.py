from lm_anal.src.datum.pcloud.pcloud import PCloud
from lm_anal.src.spec import DatumSpec as DatSpc, DatumSpecs as DatSpcs

__all__ = ['Probability']

class Probability(PCloud):
    pointsDtype = [('value', 'float'), ('probability', 'float')]
    propertySpecs = DatSpcs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='flux', targetName='points')