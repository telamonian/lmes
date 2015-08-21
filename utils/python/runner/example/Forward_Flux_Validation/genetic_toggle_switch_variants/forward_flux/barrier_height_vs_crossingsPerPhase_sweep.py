#!/usr/bin/env python
import numpy as np; np.set_printoptions(precision=1, threshold=1e6, linewidth=1e6)
import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
host = 'xanthus'
lm_bin = '/home/cklein13/git/lm/build_cuda/lmes'
local_home_directory = thisScriptsPath
maxPhaseZeroTime = str(int(1e6))
queue = 'gpu'
remote_home_directory = '/home/cklein13'
type = 'sge'
user_id = 'cklein13'
runnerPath = '/Users/tel/git/lm/utils/python/runner'
################################

sys.path.append(runnerPath)

from helper import *
from job import JobSGE
from lmFile import *
from runner import Runner
from sweep import Sweep, SweepTup

def GetFFluxInputTups():
    iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])
    iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])
    ops = [
        OrderParameter(type=0,
                       id=0,
                       speciesIDs=[0,1,2,3,4,5],
                       speciesCoefficients=[-1,-2,-2,1,2,2]),
        OrderParameter(type=0,
                       id=1,
                       speciesIDs=[0,1,2],
                       speciesCoefficients=[1,2,2]),
        OrderParameter(type=0,
                       id=2,
                       speciesIDs=[3,4,5],
                       speciesCoefficients=[1,2,2])]
    simParams = [SimulationParameter(key='maxPhaseZeroTime',val=maxPhaseZeroTime)]
    tilings = [
        Tiling(id=0,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-25,25,13),
               isCurrentTiling=True),
        Tiling(id=19,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-25,25,13)),
        Tiling(id=1,
               orderParameterID=1,
               type=0,
               edges=np.arange(100)),
        Tiling(id=2,
               orderParameterID=2,
               type=0,
               edges=np.arange(100)),
        Tiling(id=199,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-30,30,16)),
        Tiling(id=7,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-25,25,11)),
        Tiling(id=27194,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-20,20,5))]
    return [iSCs, iSCBs] + ops + simParams + tilings
#     tilings = []
#     tileCounts = list(range(0,51))[-1::-10]
#     tileCounts[-1] = 1
#     for i in tileCounts:
#         if i==50:
#             isCurrentTiling=True
#         else:
#             isCurrentTiling=False
#         tilings.append(Tiling(id=0,
#                               orderParameterID=0,
#                               type=0,
#                               edges=np.linspace(-25,25,i),
#                               isCurrentTiling=isCurrentTiling))
#     return [iSCs, iSCBs, op] + tilings + simParams

if __name__=='__main__':
    xTicks = LogTicks(0,1,base=10,resolution=4)
    yTicks = [int(1e5)] #LogTicks(3,5,base=10,resolution=0)
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15)))]
    # barrier heights get swept through in x...
    inputTupssX = []
    for theta in xTicks:
        productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=5, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=11, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
        degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta), 
                                ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
        inputTupssX.append(productionConstants + degradationConstants)
    # ...and crossingsPerPhase get swept through in y
    inputTupssY = []
    for crossingsPerPhase in yTicks:
        inputTupssY.append([SimulationParameter(key='crossingsPerPhase',val=crossingsPerPhase)])
    
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='theta%.1e', labelVals=xTicks)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='cpp%.1e', labelVals=yTicks)
    
    labelX = np.array(['theta%.1e' % xTick for xTick in xTicks])
    labelY = np.array(['cpp%.1e' % yTick for yTick in yTicks])
    print('Running jobs with these parameters:')
    print(['%s_%s' % (lx,ly) for lx in labelX for ly in labelY])
    
    sweep_dict = {'cpu_count': 15,
                  #'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lmArgsIntout': True,
                  'lmArgsGpusPerReplicate': '1/4',
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': {'rate':'auto', 'weight':.1}, #1e3
                  'lm_sampling_time': 1e10,
                  'queue': queue,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_fflux_barrier_height_vs_crossingsPerPhase_tenfold_sampling'),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
