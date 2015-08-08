import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.helper import FastVStack, Tupify
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['BasinsToPhaseWeightsPT']

class BasinsToPhaseWeightsPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'basins'})
    dstProps = frozenset({'phase_weights'})
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        ffluxDatum = srcDatum[datumABCDict['fflux']]
        
        # dtype for the resulting phase_weights array
        pwDtype = list(zip(dstDatum.propertySpecs['phase_weights']['columnLabels'], 
                           dstDatum.propertySpecs['phase_weights']['dtype']))
        
        # the sampling rate set by 'writeInterval' in SimulationParameters
        stepTime = float(kwargs['simulationParameters'].get('writeInterval'))
        
        weightParts = []
        for directionID,direction in zip((0, 1),('FORWARD', 'BACKWARD')):
            # for now, as with the rest of lmes fflux, the interface tiling has to be 1D
            weightPart = np.zeros((np.sum(kwargs['tilings'][ffluxDatum.tiling_id].dims),), dtype=pwDtype)
            for dimIndices in kwargs['tilings'][ffluxDatum.tiling_id].getEdgeIndices():
                weightPart[dimIndices[0]] = (1, directionID, 0)
                weightPart[dimIndices[1]] = (stepTime, directionID, 1)
                for i in dimIndices[2:]:
                    weightPart[i] = (stepTime*ffluxDatum.basins[direction].probability_one_to_i_plus_one[i - 1], directionID, i)
            weightParts.append(weightPart)
        weights = FastVStack(*weightParts)
        weights.sort(order=['basin','phase'])
        
        dstDatum.setArray('phase_weights', weights)
        dstDatum.setArray('interface_tiling_id', np.array(Tupify(ffluxDatum.tiling_id)))
        dstDatum.setScalar('time_step', stepTime)
        