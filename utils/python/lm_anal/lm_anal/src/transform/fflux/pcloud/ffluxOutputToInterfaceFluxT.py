from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.datum.probability import InterfaceFlux
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['FFluxOutputToInterfaceFluxT']

class FFluxOutputToInterfaceFluxT(BaseT):
    srcTypes = frozenset({FFluxOutput})
    dstTypes = frozenset({InterfaceFlux})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes={FFluxOutput}, dstTypes={InterfaceFlux}, requiredData={'tilings'},
        propertyTransformSpecs=PropertyTransformSpecs(
            PropertyTransformSpec(dstProps='interface_flux', srcProps='flux_out_of_tile_zero', type='special'))))
    
    def genDatumKeyTuples(self, ffluxOutputKey, ffluxBasinKey, **kwargs):
        '''
        (key, value) pair tuples version
        '''
        return (('InterfaceTilingID', ffluxOutputKey), ('Direction', ffluxBasinKey))

    def tfd(self, srcs, dsts, keys=None, **kwargs):
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        for pT in self.propertyTransforms:
            if len(pT.srcTypes)==1:
                srcKeyData = self.findDataFromDatumInSet(srcs, next(iter(pT.srcTypes)))
            else:
                srcKeyData = self.findDataFromDatumInSet(srcs, pT.srcKeyType)
            
            if len(pT.dstTypes)==1:
                dstKeyData = self.findDataFromDatumInSet(dsts, next(iter(pT.dstTypes)))
            else:
                dstKeyData = self.findDataFromDatumInSet(dsts, pT.dstKeyType)
                    
            if keys==None:
                keys = [(ffluxOutputKey, ffluxBasinKey) for ffluxOutputKey,val in srcKeyData.items() for ffluxBasinKey in val.basins.keys()]

            for ffluxOutputKey,ffluxBasinKey in keys:
                ffluxOutputDatum,ffluxBasinDatum = srcKeyData[ffluxOutputKey],srcKeyData[ffluxOutputKey].basins[ffluxBasinKey]
                srcsWithDatum = {ffluxOutputDatum,ffluxBasinDatum} | srcs
                datumKey = self.genDatumKeyTuples(ffluxOutputKey, ffluxBasinKey, **kwargs)
                # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
                dstDatum = dstKeyData.initDatum(datumKey, o=ffluxOutputDatum.o)
                dstsWithDatum = {dstDatum} | dsts

                pT.transformProperties(srcs=srcsWithDatum, dsts=dstsWithDatum, **kwargs)