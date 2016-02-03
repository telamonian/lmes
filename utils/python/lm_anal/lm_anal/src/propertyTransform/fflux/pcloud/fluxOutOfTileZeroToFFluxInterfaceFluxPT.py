from lm_anal.src.propertyTransform.fflux.pcloud.fluxOutOfTileZeroToInterfaceFluxPT import FluxOutOfTileZeroToInterfaceFluxPT

__all__ = ['FluxOutOfTileZeroToFFluxInterfaceFluxPT']

class FluxOutOfTileZeroToFFluxInterfaceFluxPT(FluxOutOfTileZeroToInterfaceFluxPT):
    dstProps = frozenset({'fflux_interface_flux'})

    def getDstDatum(self, dataCaddy):
        return dataCaddy.dstDict['FFluxInterfaceFlux']

    def getFieldDict(self, dataCaddy):
        fieldDict = super().getFieldDict(dataCaddy=dataCaddy)

        fieldDict['time_basinal'] = dataCaddy.ffluxBasinDatum.time_per_phase[0]
        fieldDict['time_regional'] = dataCaddy.ffluxBasinDatum.time_per_phase[1]
        fieldDict['flux_basinal'] = float(fieldDict['count'])/fieldDict['time_basinal']
        fieldDict['flux_regional'] = fieldDict.pop('flux')

        return fieldDict