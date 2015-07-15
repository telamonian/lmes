import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datum.hist import HistBase, OParamHists
from lm_anal.src.datum.fflux import FFluxBase
from lm_anal.src.transform.propertyTransform import BasePT

class FFluxOutputTrajectoriesToHistOParamValuesPT(BasePT):
    srcType = FFluxBase
    dstType = HistBase
    srcProp = 'trajectories'
    dstProp = 'order_parameter_values'
    
    def __init__(self, oparams, simParams, specTrajs, tilings, **kwargs):
        self.oparams = oparams
        self.simParams = simParams
        self.specTrajs = specTrajs
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            stepTime = float(self.simParams.simulationParameter['writeInterval'])
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            dstDatum.subDatum = OParamHists()
            dstDatum.subDatum['FORWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['BACKWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['ZERO'] = dstDatum.getCopy()
            for trajID,traj in self.specTrajs:
                try:
                    trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    direction = 'FORWARD'
                except KeyError:
                    trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    direction = 'BACKWARD'
                if trajPhase==0:
                    weight = 1.0
                    timeBoolArr = (traj.time % stepTime == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                    if np.any(timeBoolArr):
                        obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                        dstDatum.subDatum['ZERO'].addWeightedObservations(obs, weight=weight)
                    continue
                elif trajPhase==1:
                    weight = stepTime
                else:
                    weight = stepTime*srcDatum.basins[direction].probability_one_to_i_plus_one[trajPhase - 1]
                timeBoolArr = (traj.time % stepTime == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                if np.any(timeBoolArr):
                    obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                    if direction=='FORWARD':
                        dstDatum.subDatum['FORWARD'].addWeightedObservations(obs, weight=weight)
                    else:
                        dstDatum.subDatum['BACKWARD'].addWeightedObservations(obs, weight=weight)
            for direction in ('FORWARD', 'BACKWARD'):
                dstDatum.subDatum[direction].reweight(srcDatum.basins[direction].this_basin_last_visited_probability
                                                      *srcDatum.basins[direction].flux_out_of_tile_zero)
            dstDatum+=dstDatum.subDatum['FORWARD']
            dstDatum+=dstDatum.subDatum['BACKWARD']
            
            # stiching the phase zero stuff on            
            nonzeroArr = np.nonzero(dstDatum.h)
            weight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.subDatum['ZERO'], weight=x), x0=[.0001], method='Nelder-Mead')
            #weight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.subDatum['ZERO'], weight=x), x0=[.0001], method='Nelder-Mead')
            print('the weight to match phase zero is: %.8f' % weight.x)
            dstDatum.subDatum['ZERO'].reweight(weight.x)
            dstDatum.subDatum['ZERO_MASKED'] = dstDatum.subDatum['ZERO'].getCopy()
            dstDatum.subDatum['ZERO_MASKED'].h[nonzeroArr] = 0
            dstDatum+=dstDatum.subDatum['ZERO_MASKED']
        except AttributeError:
            pass