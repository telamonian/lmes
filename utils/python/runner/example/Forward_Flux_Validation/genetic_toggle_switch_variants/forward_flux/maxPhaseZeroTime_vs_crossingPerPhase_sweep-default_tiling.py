#!/usr/bin/env python

import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
host = 'kirin'
lm_bin = '/home/cklein13/git/lm/build/lmes'
local_home_directory = thisScriptsPath
remote_home_directory = '/scratch/cklein13'
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
    tiling = Tiling(id=0,
                orderParameterID=0,
                type=0,
                edges=np.linspace(-25,25,13))
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
    return [iSCs, iSCBs, op, tiling]

if __name__=='__main__':
    crossingsPerPhases = LogTicks(3,7,base=10,resolution=0).astype(int)
    maxPhaseZeroTimes = LogTicks(3,7,base=10,resolution=0).astype(int)
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val='1000000000'),
                 SimulationParameter(key='maxWorkUnitSteps',val='100000')]
    # set the protein production/degradation reaction rates to be scaled by .1, giving the system a high effective barrier between the states
    reactionRates = [ReactionRateConstant(reactionID=4, rateConstant=.1), 
                     ReactionRateConstant(reactionID=5, rateConstant=.1), 
                     ReactionRateConstant(reactionID=11, rateConstant=.1), 
                     ReactionRateConstant(reactionID=12, rateConstant=.1),
                     ReactionRateConstant(reactionID=6, rateConstant=.025), 
                     ReactionRateConstant(reactionID=13, rateConstant=.025)]
    # crossingsPerPhase get swept through in x...
    inputTupssX = [[SimulationParameter(key='crossingsPerPhase',val=str(cPP))] for cPP in crossingsPerPhases]
    # ...and maxPhaseZeroTime get swept through in y
    inputTupssY = [[SimulationParameter(key='maxPhaseZeroTime',val=str(mPZT))] for mPZT in maxPhaseZeroTimes]
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='crossingPerPhase%08d', labelVals=crossingsPerPhases)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='maxPhaseZeroTime%08d', labelVals=maxPhaseZeroTimes)

    sweep_dict = {'autosetSamplingRate': False,
                  'autosetSamplingTime': False,
                  'cpu_count': 96,
                  'diagonal': False,
                  'host': host,
                  'inputTupsDefault': simParams + reactionRates + GetFFluxInputTups(),
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation_project/fflux-maxPhaseZeroTime_vs_crossingsPerPhase_sweep-default_tiling'),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
