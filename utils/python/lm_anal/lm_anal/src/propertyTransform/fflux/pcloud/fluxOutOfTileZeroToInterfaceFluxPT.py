import numpy as np

from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['FluxOutOfTileZeroToInterfaceFluxPT']

class FluxOutOfTileZeroToInterfaceFluxPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('pcloud')
    
    srcProps = frozenset({'flux_out_of_tile_zero'})
    dstProps = frozenset({'interface_flux'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        ffluxOutputDatum = srcDict['FFluxOutput']
        ffluxBasinDatum = srcDict['FFluxBasin']
        dstDatum = dstDict['InterfaceFlux']

        tiling = kwargs['tilings'][ffluxOutputDatum.tiling_id]
        edges = np.asarray(tiling.getEdges()[0])
        interface = edges[0] if ffluxBasinDatum.direction==0 else edges[-1]
        flux = ffluxBasinDatum.flux_out_of_tile_zero

        interface_flux = np.array([(interface, flux)], dtype=dstDatum.propertySpecs['points']['dtype'])

        dstDatum.setArray('interface_flux', interface_flux)