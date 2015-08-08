import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datumABC import GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

# __all__ = ['SpeciesCountToOParamValuesPT']

class SpeciesCountToOParamValuesPT(BasePT):
    srcABCs = GetDatumStrABCSet(('fflux', 'trajectory'))
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'species_count'})
    dstProps = frozenset({'order_parameter_values'})
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        try:
            # the sampling rate set by 'writeInterval' in SimulationParameters
            stepTime = float(kwargs['simulationParameters'].get('writeInterval'))
            
            # initialize some extra histograms in which to store some intermediate data
            dstDatum.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
            dstDatum.subDatum = OParamHists()
            dstDatum.subDatum['FORWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['BACKWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['ZERO'] = dstDatum.getCopy()
            
            for trajID,traj in self.specTrajs:
                # get the forward flux phase this trajectory was launched during, and the direction (ie A->B or A<-B)
                try:
                    trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    # if trajPhase==0, we don't care about the direction per se, so just mark it as ZERO
                    if trajPhase > 0:
                        direction = 'FORWARD'
                    else:
                        direction = 'ZERO'
                except KeyError:
                    trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    if trajPhase > 0:
                        direction = 'BACKWARD'
                    else:
                        direction = 'ZERO'
                
                # get a boolean mask that will allow us to select out only those timepoints that coincide with the stepTime
                timeBoolArr = (traj.time % stepTime == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                
                # calculate the weight for this trajectory based on the fflux phase
                if trajPhase==0:
                    weight = 1
                elif trajPhase==1:
                    weight = stepTime
                else:
                    weight = stepTime*srcDatum.basins[direction].probability_one_to_i_plus_one[trajPhase - 1]
                
                # calculate the order parameter values and add them to the appropriate directional histogram (or ZERO) with the appropriate weight
                if np.any(timeBoolArr):
                    obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                    dstDatum.subDatum[direction].addWeightedObservations(obs, weight=weight)
                    
            # now that the directional hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
            for direction in ('FORWARD', 'BACKWARD'):
                dstDatum.subDatum[direction].reweight(srcDatum.basins[direction].this_basin_last_visited_probability
                                                      *srcDatum.basins[direction].flux_out_of_tile_zero)
            dstDatum+=dstDatum.subDatum['FORWARD']
            dstDatum+=dstDatum.subDatum['BACKWARD']
            
            # stitch the brute force hist obtained during phase 0 onto the fflux-derived hist
            nonzeroArr = np.nonzero(dstDatum.h)
            weight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.subDatum['ZERO'], weight=x), x0=[1], method='Nelder-Mead')
            #weight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.subDatum['ZERO'], weight=x), x0=[.0001], method='Nelder-Mead')
            print('the phase zero best fit weight is: %.8f' % weight.x)
            dstDatum.subDatum['ZERO'].reweight(weight.x)
            dstDatum.subDatum['ZERO_MASKED'] = dstDatum.subDatum['ZERO'].getCopy()
            dstDatum.subDatum['ZERO_MASKED'].h[nonzeroArr] = 0
            dstDatum+=dstDatum.subDatum['ZERO_MASKED']
        except AttributeError:
            pass
    
    def __init__(self, oparams, simParams, specTrajs, tilings, **kwargs):
        self.oparams = oparams
        self.simParams = simParams
        self.specTrajs = specTrajs
        self.tilings = tilings
    
    def __call__(self, srcDatum, dstDatum):
        try:
            # the sampling rate set by 'writeInterval' in SimulationParameters
            stepTime = float(self.simParams.simulationParameter['writeInterval'])
            
            # initialize some extra histograms in which to store some intermediate data
            dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
            dstDatum.subDatum = OParamHists()
            dstDatum.subDatum['FORWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['BACKWARD'] = dstDatum.getCopy()
            dstDatum.subDatum['ZERO'] = dstDatum.getCopy()
            
            for trajID,traj in self.specTrajs:
                # get the forward flux phase this trajectory was launched during, and the direction (ie A->B or A<-B)
                try:
                    trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    # if trajPhase==0, we don't care about the direction per se, so just mark it as ZERO
                    if trajPhase > 0:
                        direction = 'FORWARD'
                    else:
                        direction = 'ZERO'
                except KeyError:
                    trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
                    if trajPhase > 0:
                        direction = 'BACKWARD'
                    else:
                        direction = 'ZERO'
                
                # get a boolean mask that will allow us to select out only those timepoints that coincide with the stepTime
                timeBoolArr = (traj.time % stepTime == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
                
                # calculate the weight for this trajectory based on the fflux phase
                if trajPhase==0:
                    weight = 1
                elif trajPhase==1:
                    weight = stepTime
                else:
                    weight = stepTime*srcDatum.basins[direction].probability_one_to_i_plus_one[trajPhase - 1]
                
                # calculate the order parameter values and add them to the appropriate directional histogram (or ZERO) with the appropriate weight
                if np.any(timeBoolArr):
                    obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
                    dstDatum.subDatum[direction].addWeightedObservations(obs, weight=weight)
                    
            # now that the directional hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
            for direction in ('FORWARD', 'BACKWARD'):
                dstDatum.subDatum[direction].reweight(srcDatum.basins[direction].this_basin_last_visited_probability
                                                      *srcDatum.basins[direction].flux_out_of_tile_zero)
            dstDatum+=dstDatum.subDatum['FORWARD']
            dstDatum+=dstDatum.subDatum['BACKWARD']
            
            # stitch the brute force hist obtained during phase 0 onto the fflux-derived hist
            nonzeroArr = np.nonzero(dstDatum.h)
            weight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.subDatum['ZERO'], weight=x), x0=[1], method='Nelder-Mead')
            #weight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.subDatum['ZERO'], weight=x), x0=[.0001], method='Nelder-Mead')
            print('the phase zero best fit weight is: %.8f' % weight.x)
            dstDatum.subDatum['ZERO'].reweight(weight.x)
            dstDatum.subDatum['ZERO_MASKED'] = dstDatum.subDatum['ZERO'].getCopy()
            dstDatum.subDatum['ZERO_MASKED'].h[nonzeroArr] = 0
            dstDatum+=dstDatum.subDatum['ZERO_MASKED']
        except AttributeError:
            pass