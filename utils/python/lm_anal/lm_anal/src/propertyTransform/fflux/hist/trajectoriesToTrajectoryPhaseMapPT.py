import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.helper import FastVStack
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['TrajectoriesToTrajectoryPhaseMapPT']

class TrajectoriesToTrajectoryPhaseMapPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'trajectories'})
    dstProps = frozenset({'trajectory_phase_map'})
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        if not dstDatum.full:
            return
        
        ffluxDatum = srcDatum[datumABCDict['fflux']]
        
        trajectoryPhaseMapParts = [ffluxDatum.trajectories['%s/INITIAL' % direction].trajectory_phase_map for direction in ('FORWARD', 'BACKWARD')]
        trajectoryPhaseMap = FastVStack(*trajectoryPhaseMapParts)
        trajectoryPhaseMap.sort(order=['trajectory_id'])
        dstDatum.setArray('trajectory_phase_map', trajectoryPhaseMap)
        
#         trajectoryPhaseMapIndices = [0]
#         trajectoryPhaseMapShape = np.array((0,3))
#         for direction in ('FORWARD', 'BACKWARD'):
#             trajectoryPhaseMapIndices.append(ffluxDatum.trajectories['%/INITIAL' % direction].trajectory_phase_map.shape[0])
#             trajectoryPhaseMapShape[0]+=ffluxDatum.trajectories['%/INITIAL' % direction].trajectory_phase_map.shape[0]
#         trajectoryPhaseMapIndexTuples = zip(trajectoryPhaseMapIndices, trajectoryPhaseMapIndices[1:])
#         
#         ffluxDatum.trajectory_phase_map = np.zeros(trajectoryPhaseMapShape)
#         for (start,end),direction in (trajectoryPhaseMapIndexTuples, ('FORWARD', 'BACKWARD')):
#             ffluxDatum.trajectory_phase_map[start:end,:] = ffluxDatum.trajectories['%/INITIAL' % direction].trajectory_phase_map