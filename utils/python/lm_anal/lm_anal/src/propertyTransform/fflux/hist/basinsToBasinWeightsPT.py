import numpy as np

from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['BasinsToBasinWeightsPT']

class BasinsToBasinWeightsPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'basins'})
    dstProps = frozenset({'basin_weights'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        ffluxDatum = srcDict['FFluxOutput']
        dstDatum = dstDict['FFluxHist']
        
        # dtype for the resulting phase_weights array
        bwDtype = list(zip(dstDatum.propertySpecs['basin_weights']['columnLabels'], 
                           dstDatum.propertySpecs['basin_weights']['dtype']))
        
        weights = np.zeros((2,), dtype=dstDatum.propertySpecs['basin_weights']['dtype'])
        for directionID,direction in zip((0, 1),('FORWARD', 'BACKWARD')):
            weights[directionID] = (ffluxDatum.basins[direction].this_basin_last_visited_probability
                                   *ffluxDatum.basins[direction].flux_out_of_tile_zero, 
                                    directionID)
        weights.sort(order=['basin_id'])
        
        dstDatum.setArray('basin_weights', weights)
        