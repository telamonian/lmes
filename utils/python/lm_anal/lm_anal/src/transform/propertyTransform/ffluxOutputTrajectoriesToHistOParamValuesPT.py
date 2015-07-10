import numpy as np

from lm_anal.src.datum.hist import HistBase
from lm_anal.src.datum.fflux import FFluxBase
from lm_anal.src.transform.propertyTransform import BasePT

class FFluxOutputTrajectoriesToHistOParamValuesPT(BasePT):
    srcType = FFluxBase
    dstType = HistBase
    srcProp = 'trajectories'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparams, specTrajs, tilings, **kwargs):
        self.oparams = oparams
        self.specTrajs = specTrajs
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            backwardDatum = dstDatum.getCopy()
            oD = dstDatum.getCopy()
            for trajID,traj in self.specTrajs:
                try:
                    trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    direction = 'FORWARD'
                except KeyError:
                    trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    direction = 'BACKWARD'
                if trajPhase==0:
                    weight = 1.0
                    timeBoolArr = (traj.time % 4.0 == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                    if np.any(timeBoolArr):
                        obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                        oD.addWeightedObservations(obs, weight=weight)
                    continue
                elif trajPhase==1:
                    weight = 4.0
                else:
#                     if not isinstance(trajPhase - 1, int):
#                         print(trajPhase - 1)
                    weight = 4*srcDatum.basins[direction].probability_one_to_i_plus_one[trajPhase - 1]
                timeBoolArr = (traj.time % 4.0 == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                if np.any(timeBoolArr):
                    obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                    if direction=='FORWARD':
                        dstDatum.addWeightedObservations(obs, weight=weight)
                    else:
                        backwardDatum.addWeightedObservations(obs, weight=weight)
            for datum,direction in zip((dstDatum, backwardDatum), ('FORWARD', 'BACKWARD')):
                datum.reweight(srcDatum.basins[direction].this_basin_last_visited_probability
                              *srcDatum.basins[direction].flux_out_of_tile_zero)
            dstDatum+=backwardDatum
            
            nonzeroArr = np.nonzero(dstDatum.h)
            weight = (dstDatum.h[1,26] + dstDatum.h[26,1])/(oD.h[1,26] + oD.h[26,1])
            print('the weight to match phase zero is: %.8f' % weight)
            oD.reweight(weight)
            oD.h[nonzeroArr] = 0
            dstDatum+=oD
        except AttributeError:
            pass