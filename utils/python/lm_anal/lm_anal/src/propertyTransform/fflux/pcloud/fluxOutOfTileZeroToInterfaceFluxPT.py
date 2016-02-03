from lm_anal.src.propertyTransform.fflux.pcloud.ffluxPropToPCloudPropPT import FFluxPropToPCloudPropPT

__all__ = ['FluxOutOfTileZeroToInterfaceFluxPT']

class FluxOutOfTileZeroToInterfaceFluxPT(FFluxPropToPCloudPropPT):
    srcProps = frozenset({'flux_out_of_tile_zero'})
    dstProps = frozenset({'interface_flux'})

    def getDstDatum(self, dataCaddy):
        return dataCaddy.dstDict['InterfaceFlux']

    def getFieldDict(self, dataCaddy):
        return {'interface': dataCaddy.edges[0],
                'count': dataCaddy.ffluxBasinDatum.runs_per_phase[0],
                'flux': dataCaddy.ffluxBasinDatum.flux_out_of_tile_zero}

    def getPointsShape(self, fieldDict):
        return 1