from lm_anal.src.datum.pcloud.pcloud import PCloud
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['InterfaceFlux']

class InterfaceFlux(PCloud):
    pointsDtype = [('interface', 'float'), ('flux', 'float')]
    propertySpecs = DatumSpecs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='interface_flux', targetName='points')