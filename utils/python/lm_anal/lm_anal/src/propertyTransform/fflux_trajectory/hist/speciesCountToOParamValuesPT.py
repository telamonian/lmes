import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datum.hist import FFluxHist
from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['SpeciesCountToOParamValuesPT']

directionDict = {0:'FORWARD', 1:'BACKWARD'}

class SpeciesCountToOParamValuesPT(BasePT):
    srcABCs = GetDatumStrABCSet(('fflux', 'trajectory'))
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'species_count'})
    dstProps = frozenset({'order_parameter_values'})
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        if not dstDatum.full:
            return
        
        trajData = srcDatum[datumABCDict['trajectory']]
        
        # set the ffluxHist's tiling
        dstDatum.setArray('bin_tiling_id', np.array(kwargs['tilingIDs']))
        dstDatum.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
        
        # initialize some extra histograms in which to store some intermediate data
        subFFluxHist = FFluxHist()
        subFFluxHist.setArray('phase_weights', dstDatum.phase_weights)
        subFFluxHist.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
        dstDatum.phase_zero_order_parameter_values['FORWARD'] = subFFluxHist.getCopy()
        dstDatum.phase_zero_order_parameter_values['BACKWARD'] = subFFluxHist.getCopy()
        dstDatum.phase_n_order_parameter_values['FORWARD'] = subFFluxHist.getCopy()
        dstDatum.phase_n_order_parameter_values['BACKWARD'] = subFFluxHist
        
        basinWeightDict = {bwRow['basin_id']:bwRow['weight'] for bwRow in dstDatum.basin_weights}
        phaseWeightDict = {(pwRow['basin_id'], pwRow['phase_id']):pwRow['weight'] for pwRow in dstDatum.phase_weights}
        
        foundFFONotFoundST = []
        for tpmRow in dstDatum.trajectory_phase_map:
            # in certain cases (mostly at the end of phases), a trajectory may get recorded in ffluxOutput without every actually running and getting its own speciesTrajectory
            if int(tpmRow['trajectory_id']) not in trajData:
                foundFFONotFoundST.append(int(tpmRow['trajectory_id']))
                continue
            
            # get the trajectory that corresponds to this row of the trajectory_phase_map
            traj = trajData[tpmRow['trajectory_id']]
            
            # get a boolean mask that will allow us to select out only those timepoints that coincide with the stepTime
            timeBoolArr = (traj.time % dstDatum.time_step == 0)
            # make sure that the initial and final timepoints are masked out in any case
#             timeBoolArr[0] = False; timeBoolArr[-1] = False
            # if all of the timepoints are masked, skip this trajectory
            if not np.any(timeBoolArr):
                continue
            
            # based on the basin and phase of traj, get the appropriate sub hist and weight
            phaseStr = 'zero' if tpmRow['phase_id']==0 else 'n'
            direction = directionDict[tpmRow['basin_id']]
            subFFluxHist = dstDatum.__getattribute__('phase_%s_order_parameter_values' % phaseStr)[direction]
            
            weight = phaseWeightDict[(tpmRow['basin_id'], tpmRow['phase_id'])]
            
            # calculate the order parameter values and add them to the appropriate sub histogram with the appropriate weight
            obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
            subFFluxHist.addWeightedObservations(obs, weight=weight)
        
        # if anything ends up in foundFFONotFoundST, maybe warn about it?
        print('trajectories {} found in ffluxOutput but not in speciesTrajectories'.format(
              ('{:d},'*len(foundFFONotFoundST)).format(*foundFFONotFoundST).rstrip(',')))
        
        # now that the sub hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
        for directionID,direction in directionDict.items():
            dstDatum.phase_n_order_parameter_values[direction].reweight(basinWeightDict[directionID])
        dstDatum.combineInPlace(dstDatum.phase_n_order_parameter_values['FORWARD'], 
                                dstDatum.phase_n_order_parameter_values['BACKWARD'])
            
        # stitch the brute force hist obtained during phase 0 onto the fflux-derived hist
#         nonzeroMask = np.nonzero(dstDatum.h)==False
        nonzeroMask = dstDatum.h!=0
        for direction in directionDict.values():
            phaseZeroWeight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.phase_zero_order_parameter_values[direction], weight=x), x0=[1], method='Nelder-Mead')
            #phaseZeroWeight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.phase_zero_order_parameter_values[direction], weight=x), x0=[.0001], method='Nelder-Mead')
            print('the %s phase zero best fit weight is: %.8f' % (direction, phaseZeroWeight.x))
            dstDatum.phase_zero_order_parameter_values[direction].reweight(phaseZeroWeight.x)
            dstDatum.phase_zero_order_parameter_values[direction].remask(nonzeroMask)
        dstDatum.combineInPlace(dstDatum.phase_zero_order_parameter_values['FORWARD'],
                                dstDatum.phase_zero_order_parameter_values['BACKWARD'])
        
        # normalize the whole schlemiel
        dstDatum.normalize()
    
#     def __init__(self, oparams, simParams, specTrajs, tilings, **kwargs):
#         self.oparams = oparams
#         self.simParams = simParams
#         self.specTrajs = specTrajs
#         self.tilings = tilings
#     
#     def __call__(self, srcDatum, dstDatum):
#         try:
#             # the sampling rate set by 'writeInterval' in SimulationParameters
#             stepTime = float(self.simParams.simulationParameter['writeInterval'])
#             
#             # initialize some extra histograms in which to store some intermediate data
#             dstDatum.setTilings(oparams=self.oparams, tilings=self.tilings)
#             dstDatum.subDatum = OParamHists()
#             dstDatum.subDatum['FORWARD'] = dstDatum.getCopy()
#             dstDatum.subDatum['BACKWARD'] = dstDatum.getCopy()
#             dstDatum.subDatum['ZERO'] = dstDatum.getCopy()
#             
#             for trajID,traj in self.specTrajs:
#                 # get the forward flux phase this trajectory was launched during, and the direction (ie A->B or A<-B)
#                 try:
#                     trajPhase = srcDatum.trajectories['FORWARD/INITIAL'].trajectoryPhaseMap[trajID]
#                     # if trajPhase==0, we don't care about the direction per se, so just mark it as ZERO
#                     if trajPhase > 0:
#                         direction = 'FORWARD'
#                     else:
#                         direction = 'ZERO'
#                 except KeyError:
#                     trajPhase = srcDatum.trajectories['BACKWARD/INITIAL'].trajectoryPhaseMap[trajID]
#                     if trajPhase > 0:
#                         direction = 'BACKWARD'
#                     else:
#                         direction = 'ZERO'
#                 
#                 # get a boolean mask that will allow us to select out only those timepoints that coincide with the stepTime
#                 timeBoolArr = (traj.time % stepTime == 0); timeBoolArr[0] = False; timeBoolArr[-1] = False
#                 
#                 # calculate the weight for this trajectory based on the fflux phase
#                 if trajPhase==0:
#                     weight = 1
#                 elif trajPhase==1:
#                     weight = stepTime
#                 else:
#                     weight = stepTime*srcDatum.basins[direction].probability_one_to_i_plus_one[trajPhase - 1]
#                 
#                 # calculate the order parameter values and add them to the appropriate directional histogram (or ZERO) with the appropriate weight
#                 if np.any(timeBoolArr):
#                     obs = dstDatum.oparam.calc(traj.species_count[timeBoolArr])
#                     dstDatum.subDatum[direction].addWeightedObservations(obs, weight=weight)
#                     
#             # now that the directional hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
#             for direction in ('FORWARD', 'BACKWARD'):
#                 dstDatum.subDatum[direction].reweight(srcDatum.basins[direction].this_basin_last_visited_probability
#                                                       *srcDatum.basins[direction].flux_out_of_tile_zero)
#             dstDatum+=dstDatum.subDatum['FORWARD']
#             dstDatum+=dstDatum.subDatum['BACKWARD']
#             
#             # stitch the brute force hist obtained during phase 0 onto the fflux-derived hist
#             nonzeroArr = np.nonzero(dstDatum.h)
#             weight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.subDatum['ZERO'], weight=x), x0=[1], method='Nelder-Mead')
#             #weight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.subDatum['ZERO'], weight=x), x0=[.0001], method='Nelder-Mead')
#             print('the phase zero best fit weight is: %.8f' % weight.x)
#             dstDatum.subDatum['ZERO'].reweight(weight.x)
#             dstDatum.subDatum['ZERO_MASKED'] = dstDatum.subDatum['ZERO'].getCopy()
#             dstDatum.subDatum['ZERO_MASKED'].h[nonzeroArr] = 0
#             dstDatum+=dstDatum.subDatum['ZERO_MASKED']
#         except AttributeError:
#             pass