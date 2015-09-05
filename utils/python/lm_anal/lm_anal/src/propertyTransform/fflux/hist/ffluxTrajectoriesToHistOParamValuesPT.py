from collections import OrderedDict
from itertools import chain
import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datum.hist import FFluxHist
from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['FFluxTrajectoriesToOParamValuesPT']

# directionDict = OrderedDict(((0,'FORWARD'), (1,'BACKWARD')))

class FFluxTrajectoriesToOParamValuesPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('hist')
    
    srcProps = frozenset({'trajectories'})
    dstProps = frozenset({'order_parameter_values'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        srcDatum = srcDict['FFluxOutput']
        dstDatum = dstDict['FFluxHist']
        if not dstDatum.full:
            return
        
#         basinWeightDict = {bwRow['basin_id']:bwRow['weight'] for bwRow in dstDatum.basin_weights}
#         phaseWeightDict = {(pwRow['basin_id'], pwRow['phase_id']):pwRow['weight'] for pwRow in dstDatum.phase_weights}
        
        # set the ffluxHist's tiling
        dstDatum.setArray('bin_tiling_id', np.array(kwargs['tilingIDs']))
        dstDatum.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
        
        # initialize some extra histograms in which to store some intermediate data
        dstDatum.initSubHists(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
#         subFFluxHist = FFluxHist(full=True)
#         subFFluxHist.setArray('basin_weights', dstDatum.basin_weights)
#         subFFluxHist.setArray('phase_weights', dstDatum.phase_weights)
#         subFFluxHist.setTilings(oparams=kwargs['oparams'], tilings=kwargs['tilings'], tilingIDs=kwargs['tilingIDs'])
#         dstDatum.phase_zero_order_parameter_values['FORWARD'] = subFFluxHist.getCopy()
#         dstDatum.phase_zero_order_parameter_values['BACKWARD'] = subFFluxHist.getCopy()
#         dstDatum.phase_n_order_parameter_values['FORWARD'] = subFFluxHist.getCopy()
#         dstDatum.phase_n_order_parameter_values['BACKWARD'] = subFFluxHist
        
        # add all of the observations for each phase to the appropriate subHist
        dstDatum.addObservationsToSubHists(ffoDatum=srcDatum, oparams=kwargs['oparams'], tilings=kwargs['tilings'])
#         for directionID,direction in directionDict.items():
#             runsPerPhaseList = srcDatum.basins['%s' % direction].runs_per_phase
#             trajs = srcDatum.trajectories['%s/RUNNING' % direction]
#             
#             phaseChangeIndices = (trajs.edge_id[1:] - trajs.edge_id[:-1]).nonzero()[0] + 1
#             for start,end in zip(chain([None], phaseChangeIndices), chain(phaseChangeIndices, [None])):
#                 phaseID = trajs.edge_id[start if start is not None else 0]
#                 phaseStr = 'zero' if phaseID==0 else 'n'
#                 subFFluxHist = dstDatum.__getattribute__('phase_%s_order_parameter_values' % phaseStr)[direction]
# #                 runsPerPhase = trajs.trajectory_id[start:end].max() - trajs.trajectory_id[start:end].min() + 1
#                 runsPerPhase = 1 if phaseID==0 else float(runsPerPhaseList[phaseID])
#                 weight = phaseWeightDict[(directionID, phaseID)] / runsPerPhase
# #                 print('phaseID: %d, runsPerPhase: %d, weight: %.3e' % (phaseID,runsPerPhase,weight))
#                 
#                 obs = dstDatum.oparam.calc(trajs.species_count[start:end])
#                 subFFluxHist.addWeightedObservations(obs, weight=weight)
            
        # now that the sub hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
        dstDatum.genBasinHists()
        dstDatum.combineBasinHists()
#         for directionID,direction in directionDict.items():
#             dstDatum.phase_n_order_parameter_values[direction].reweight(basinWeightDict[directionID])
#         dstDatum.combineInPlace(dstDatum.phase_n_order_parameter_values['FORWARD'], 
#                                 dstDatum.phase_n_order_parameter_values['BACKWARD'])

        # stitch the brute force hist obtained during phase 0 onto the fflux-derived hist
        dstDatum.weightPhaseZeroHists()
        dstDatum.stitchPhaseZeroHists()
#         nonzeroMask = np.nonzero(dstDatum.h)==False
#         nonzeroMask = dstDatum.h!=0
# #         dstDatum.scaleWeight(1e6)
#         for directionID,direction in directionDict.items():
# #             dstDatum.phase_zero_order_parameter_values[direction].rethreshold(1)
#             nearLambdaZeroMask = np.ones(dstDatum.h.shape, dtype=bool)
#             sel = [np.s_[:], np.s_[:]]
#             sel[directionID] = np.s_[24:]
#             sel = tuple(sel)
#             nearLambdaZeroMask[sel] = False
#             dstDatum.phase_zero_order_parameter_values[direction].remask(nearLambdaZeroMask)
#             nmOpt = {'disp': True}
#             reweighter = np.sum(dstDatum.h_raw)/np.sum(dstDatum.phase_zero_order_parameter_values[direction].h_raw)
#             print(reweighter)
#             dstDatum.phase_zero_order_parameter_values[direction].scaleWeight(reweighter)
#             phaseZeroWeight = minimize(lambda x: dstDatum.getWeightedRMSD(dstDatum.phase_zero_order_parameter_values[direction], weight=x), x0=1, method='Nelder-Mead')
# #             phaseZeroWeight = minimize(lambda x: dstDatum.getWeightedKLDivergence(dstDatum.phase_zero_order_parameter_values[direction], weight=x, absolute=True), x0=[1e-8], method='Nelder-Mead')
#             print('the %s phase zero best fit weight is: %.8f' % (direction, phaseZeroWeight.x))
#             dstDatum.phase_zero_order_parameter_values[direction].scaleWeight(phaseZeroWeight.x)
#             newMask = nonzeroMask.copy()
#             sel = [np.s_[:], np.s_[:]]
#             sel[not directionID] = np.s_[24:]
#             sel = tuple(sel)
#             newMask[sel] = True
#             dstDatum.phase_zero_order_parameter_values[direction].remask(newMask)
#         dstDatum.combineInPlace(dstDatum.phase_zero_order_parameter_values['FORWARD'],
#                                 dstDatum.phase_zero_order_parameter_values['BACKWARD'])
        
        # normalize the whole schlemiel
        dstDatum.normalize()