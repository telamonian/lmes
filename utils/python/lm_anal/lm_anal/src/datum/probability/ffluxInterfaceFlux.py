from lm_anal.src.datum.probability.interfaceFlux import InterfaceFlux
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['FFluxInterfaceFlux']

class FFluxInterfaceFlux(InterfaceFlux):
    pointsDtype = [('interface', 'float'),
                   ('count', 'int'),
                   ('time_basinal', 'float'),
                   ('time_regional', 'float'),
                   ('flux_basinal', 'float'),
                   ('flux_regional', 'float')]
    propertySpecs = DatumSpecs()

    # add an alias for .points in order to differentiate it from the .points property of all the other pcloud Datums during transforms
    propertySpecs.addAlias(name='fflux_interface_flux', targetName='points')

    # add some field aliases pointing to the "true" time and flux, the regional ones
    propertySpecs.addFieldAlias(name='time', targetField='time_regional', targetName='points')
    propertySpecs.addFieldAlias(name='flux', targetField='flux_regional', targetName='points')