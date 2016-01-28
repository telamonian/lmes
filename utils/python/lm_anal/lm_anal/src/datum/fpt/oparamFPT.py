from lm_anal.src.datum.fpt.fpt import FPT
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['OParamFPT']

class OParamFPT(FPT):
    # change name of base FPT 'id' field to 'oparam_id', and type of 'count' field to float
    pointsDtype = [('oparam_id', 'int'), ('count', 'float'), ('initial_count', 'float'), ('time', 'float')]
    propertySpecs = DatumSpecs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='oparam_fpt', targetName='points')