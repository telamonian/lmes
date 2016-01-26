from lm_anal.src.datum.hist.oparamHist import OParamHist
from lm_anal.src.spec import DatumSpec,DatumSpecs

__all__ = ['FFluxInterfaceHist']

class FFluxInterfaceHist(OParamHist):
    propertySpecs = DatumSpecs()

    propertySpecs.addAlias(name='fflux_interface_hist', targetName='h')