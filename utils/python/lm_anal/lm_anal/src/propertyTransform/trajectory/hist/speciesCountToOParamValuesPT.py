from lm_anal.src.propertyTransform.basePT import BasePT

#__all__ = ['SpeciesCountToHistOParamValuesPT']

class SpeciesCountToHistOParamValuesPT(BasePT):
#     srcABCs = datumABCDict['trajectory']
#     dstABCs = {datumABCDict['hist']}
    srcProps = {'species_count'}
    dstProps = {'order_parameter_values'}
    
    def __init__(self, oparams, tilings, **kwargs):
        self.oparams = oparams
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            dstDatum.setObservations(dstDatum.oparam.calc(srcDatum.__getattribute__(self.srcProp)))
        except AttributeError:
            pass