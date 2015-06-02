#!/usr/bin/env python

import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
crossingsPerPhase = str(int(1e5))
host = 'xanthus'
lm_bin = '/home/cklein13/git/lm/build_cuda/lmes'
local_home_directory = thisScriptsPath
maxPhaseZeroTime = str(int(1e7))
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
    op = OrderParameter(type=0,
                        id=0,
                        speciesIDs=[0,1,2,3,4,5],
                        speciesCoefficients=[-1,-2,-2,1,2,2])
    simParams = [SimulationParameter(key='crossingsPerPhase',val=crossingsPerPhase),
                 SimulationParameter(key='maxPhaseZeroTime',val=maxPhaseZeroTime)]
    tiling = Tiling(id=0,
                    orderParameterID=0,
                    type=0,
                    edges=np.linspace(-25,25,13))
    return [iSCs, iSCBs, op, tiling] + simParams
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
    xTicks = LogTicks(-1,-1,base=10,resolution=4)
    yTicks = xTicks/4.0
    # production reaction rates get swept through in x...
    productionRates = [[ReactionRateConstant(reactionID=4, rateConstant=tick), ReactionRateConstant(reactionID=5, rateConstant=tick), ReactionRateConstant(reactionID=11, rateConstant=tick), ReactionRateConstant(reactionID=12, rateConstant=tick)] for tick in xTicks][0]
    # ...and degradation reaction rates get swept through in y
    degradationRates = [[ReactionRateConstant(reactionID=6, rateConstant=tick), ReactionRateConstant(reactionID=13, rateConstant=tick)] for tick in yTicks][0]
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10)))]
    
    xTicks = LogTicks(8,4,base=10,resolution=0)
    inputTupssX = [[SimulationParameter(key='maxWorkUnitSteps',val=str(tick))] for tick in xTicks] 
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='maxWorkUnitSteps%.0e', labelVals=xTicks)
    # laziness WOoooOOooh! when later fixing this up (TODO), remember to take the 'diagonal' argument out of the sweep_dict
    inputTupssY = [[SimulationParameter(key='maxWorkUnitSteps',val=str(tick))] for tick in xTicks] 
    sweepTupY = SweepTup(inputTupss=inputTupssX, label='%s', labelVals=['' for tick in xTicks])

    sweep_dict = {'cpu_count': 16,
                  'diagonal': True,
                  'host': host,
                  'inputTupsDefault': degradationRates + GetFFluxInputTups() + productionRates + simParams,
                  'lmArgsIntout': True,
                  'lmArgsGpusPerReplicate': '1/4',
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 1e3, #'auto',
                  'lm_sampling_time': 1e10,
                  'queue': queue,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_fflux_fixed_maxWorkUnitSteps'),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
