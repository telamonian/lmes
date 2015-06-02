#!/usr/bin/env python

import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
crossingsPerPhase = str(int(1e5))
host = 'xanthus'
lm_bin = '/home/cklein13/git/lm/build/lmes'
local_home_directory = thisScriptsPath
maxPhaseZeroTime = str(int(1e7))
queue = 'smp-1'
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
    xTicks = LogTicks(-1,1,base=10,resolution=4)
#     xTicks = xTicks[6:7]
    yTicks = xTicks/4.0
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15)))]
    # production reaction rates get swept through in x...
    inputTupssX = [[ReactionRateConstant(reactionID=4, rateConstant=tick), ReactionRateConstant(reactionID=5, rateConstant=tick), ReactionRateConstant(reactionID=11, rateConstant=tick), ReactionRateConstant(reactionID=12, rateConstant=tick)] for tick in xTicks]
    # ...and degradation reaction rates get swept through in y
    inputTupssY = [[ReactionRateConstant(reactionID=6, rateConstant=tick), ReactionRateConstant(reactionID=13, rateConstant=tick)] for tick in yTicks]
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='production%.5f', labelVals=xTicks)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='degradation%.5f', labelVals=yTicks)

    sweep_dict = {'cpu_count': 30,
                  'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lmArgsIntout': True,
#                   'lmArgsGpusPerReplicate': '1/4',
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 1e3, #'auto',
                  'lm_sampling_time': 1e10,
                  'queue': queue,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_fflux_cpp_%.0e_mpzt_%.0e' % (int(crossingsPerPhase), int(maxPhaseZeroTime))),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
