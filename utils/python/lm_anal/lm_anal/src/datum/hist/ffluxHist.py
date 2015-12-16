from collections import OrderedDict
from itertools import chain
import numpy as np
from scipy.optimize import minimize

from lm_anal.src.datum.hist.oparamHist import OParamHist
from lm_anal.src.datum.hist.oparamHists import OParamHists
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['FFluxHist']

class FFluxHist(OParamHist):
    propertySpecs = DatumSpecs(
        DatumSpec(name='basin_n_order_parameter_values', paths=('phase_n_order_parameter_values',), SubDataType=OParamHists, type='subData'),
        DatumSpec(name='basin_weights', dtype=[('weight','float'),('basin_id','int')], paths=('basin_weights',), storageType='numpy', type='array'),
        DatumSpec(name='bin_tiling_id', dtype='int', paths=('bin_tiling_id',), storageType='numpy', type='array'),
        DatumSpec(name='interface_tiling_id', dtype='int', paths=('interface_tiling_id',), storageType='numpy', type='array'),
        DatumSpec(name='phase_weights', dtype=[('weight','float'),('basin_id','int'),('phase_id','int')], paths=('basin_weights',), storageType='numpy', type='array'),
        DatumSpec(name='phase_n_order_parameter_values', paths=('phase_n_order_parameter_values',), SubDataType=OParamHists, type='subData'),
        DatumSpec(name='time_step', dtype='float', paths=('time_step',), storageType='default', type='scalar'),
        DatumSpec(name='trajectory_phase_map', dtype=[('trajectory_id','int'),('basin_id','int'),('phase_id','int')], paths=('trajectory_phase_map',), storageType='numpy', type='array')
    )
    
    # add some alias specs
    propertySpecs.addAlias(name='phase_0_order_parameter_values', targetName='phase_zero_order_parameter_values')
      
    def __init__(self, full=False):
        super().__init__(full=full)
        
        self.initSubData()

# transform related stuff     
    def initSubHists(self, oparams, tilings, tilingIDs):
        # initialize some extra histograms in which to store some intermediate data
        subFFluxHist = OParamHist(full=True)
        subFFluxHist.setTilings(oparams=oparams, tilings=tilings, tilingIDs=tilingIDs)
        directionDict = {0:'FORWARD',1:'BACKWARD'}
        for pwRow in self.phase_weights: 
            self.phase_n_order_parameter_values[('basin_id', pwRow['basin_id']), ('phase_id', pwRow['phase_id'])] = subFFluxHist.getCopy()
#             if pwRow['basin_id'] in directionDict:
#                 self.phase_n_order_parameter_values['%s/%s' % (directionDict[pwRow['basin_id']], pwRow['phase_id'])] = self.phase_n_order_parameter_values['%s/%s' % pwRow[1:]]
        for bwRow in self.basin_weights:
            self.basin_n_order_parameter_values[('basin_id', bwRow['basin_id'])] = subFFluxHist.getCopy()
#             if bwRow['basin_id'] in directionDict: 
#                 self.basin_n_order_parameter_values['%s' % directionDict[bwRow['basin_id']]] = self.basin_n_order_parameter_values['%s' % bwRow['basin_id']]
    
    def addObservationsToSubHists(self, ffoDatum, oparams, tilings):
        # add all of the observations for each phase to the appropriate subHist
        directionDict = OrderedDict(((0,'FORWARD'), (1,'BACKWARD')))
        phaseWeightDict = {(pwRow['basin_id'], pwRow['phase_id']):pwRow['weight'] for pwRow in self.phase_weights}
        
        for directionID,direction in directionDict.items():
            runsPerPhaseList = ffoDatum.basins['%s' % direction].runs_per_phase
            trajs = ffoDatum.trajectories['%s/RUNNING' % direction]
            
            phaseChangeIndices = (trajs.edge_id[1:] - trajs.edge_id[:-1]).nonzero()[0] + 1
            for start,end in zip(chain([None], phaseChangeIndices), chain(phaseChangeIndices, [None])):
                phaseID = trajs.edge_id[start if start is not None else 0]
                subFFluxHist = self.phase_n_order_parameter_values[('basin_id', directionID), ('phase_id', phaseID)]
#                 runsPerPhase = trajs.trajectory_id[start:end].max() - trajs.trajectory_id[start:end].min() + 1
                runsPerPhase = 1 if phaseID==0 else float(runsPerPhaseList[phaseID])
                weight = phaseWeightDict[(directionID, phaseID)] / runsPerPhase
#                 print('phaseID: %d, runsPerPhase: %d, weight: %.3e' % (phaseID,runsPerPhase,weight))
                
                obs = self.oparam.calc(trajs.species_count[start:end])
                if phaseID==0:
                    fancySlice = self.getPhaseZeroObsMaskingSlice(directionID=directionID, oparams=oparams, pzObs=obs, speciesCounts=trajs.species_count[start:end], tilings=tilings)
                    obs = obs[list(fancySlice)]
                # skip trying to add observations that got completely masked out
                if obs.shape[0] > 0:
                    subFFluxHist.addWeightedObservations(obs, weight=weight)

    def combineBasinHists(self):
        self.combineInPlace(*list(self.basin_n_order_parameter_values.values()))

    def getPhaseZeroObsMaskingSlice(self, directionID, oparams, pzObs, speciesCounts, tilings, cutoffInterface=3):
        arrangmentDict = OrderedDict(((0,'ASCENDING'), (1,'DESCENDING')))
        interfaceTiling = tilings[self.interface_tiling_id[0]]
        
        if arrangmentDict[interfaceTiling.arrangement[0]]=='ASCENDING':
            if directionID==0:
                cmpStr = 'le'
                interfaceID = cutoffInterface
            elif directionID==1:
                cmpStr = 'ge'
                interfaceID = -1 - cutoffInterface
        elif arrangmentDict[interfaceTiling.arrangement[0]]=='DESCENDING':
            if directionID==0:
                cmpStr = 'ge'
                interfaceID = cutoffInterface
            elif directionID==1:
                cmpStr = 'le'
                interfaceID = -1 - cutoffInterface
        
        interfaceOParamPZObs = oparams[interfaceTiling.order_parameter_id].calc(speciesCounts)
        boolSlice = interfaceOParamPZObs.__getattribute__('__%s__' % cmpStr)(interfaceTiling.edges[interfaceID])
        fancySlice = boolSlice.nonzero()[0]
        return fancySlice
    
    def genBasinHists(self):
        # now that the sub hists have been assembled, finish weighting them according to Valeriani, 2007 eq 5 and then combine them
        directionDict = OrderedDict(((0,'FORWARD'), (1,'BACKWARD')))
        basinWeightDict = {bwRow['basin_id']:bwRow['weight'] for bwRow in self.basin_weights}
        
        for directionID,direction in directionDict.items():
            subHists= []
            for key,hist in self.phase_n_order_parameter_values.items():
#                 histBasinID,histPhaseID = [int(val) for val in key.split('/')]
#                 if directionID==histBasinID and histPhaseID!=0: 
                keyDict = dict(key)
                if keyDict['basin_id']==directionID and keyDict['phase_id']!=0: 
                    subHists.append(hist)
            self.basin_n_order_parameter_values[('basin_id', directionID)].combineInPlace(*subHists)
            self.basin_n_order_parameter_values[('basin_id', directionID)].reweight(basinWeightDict[directionID])

    def stitchPhaseZeroHists(self):
        nonzeroMask = self.h!=0

        pzHists= []
        for key,hist in self.phase_n_order_parameter_values.items():
            #             histBasinID,histPhaseID = key.split('/')
            #             if histPhaseID!=0:
            keyDict = dict(key)
            if keyDict['phase_id']==0:
                hist.remask(nonzeroMask)
                pzHists.append(hist)
        self.combineInPlace(*pzHists)

    def weightPhaseZeroHists(self):
        directionDict = OrderedDict(((0,'FORWARD'), (1,'BACKWARD')))
        
        for directionID,direction in directionDict.items():
            pzKey = ('basin_id', directionID), ('phase_id', 0)
            reweighter = np.sum(self.h_raw)/np.sum(self.phase_n_order_parameter_values[pzKey].h_raw)
            print(reweighter)
            self.phase_n_order_parameter_values[pzKey].scaleWeight(reweighter)
            phaseZeroWeight = minimize(lambda x: self.getWeightedRMSD(self.phase_n_order_parameter_values[pzKey], weight=x), x0=1, method='Nelder-Mead')
#             phaseZeroWeight = minimize(lambda x: self.getWeightedKLDivergence(self.phase_zero_order_parameter_values[direction], weight=x, absolute=True), x0=[1e-8], method='Nelder-Mead')
            print('the %s phase zero best fit weight is: %.8f' % (direction, phaseZeroWeight.x))
            self.phase_n_order_parameter_values[pzKey].scaleWeight(phaseZeroWeight.x)