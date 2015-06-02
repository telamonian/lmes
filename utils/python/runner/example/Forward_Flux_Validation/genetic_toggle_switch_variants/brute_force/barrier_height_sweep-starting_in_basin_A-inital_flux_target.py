#!/usr/bin/env python

import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
host = 'xanthus'
lm_bin = '/home/cklein13/git/lm/build_cuda/lmes'
local_home_directory = thisScriptsPath
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
    simParams = [SimulationParameter(key='crossingsPerPhase',val='1000'),
                 SimulationParameter(key='maxPhaseZeroTime',val='100000')]
    tiling = Tiling(id=0,
                    orderParameterID=0,
                    type=0,
                    edges=np.linspace(-25,25,13))
    return [iSCs, iSCBs, op, tiling] + simParams

if __name__=='__main__':
    xTicks = LogTicks(1,-1,base=10,resolution=4)
    xTicks = xTicks[1:2]
    yTicks = xTicks/4.0
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='fptTrackingList',val='0,1,2,3,4,5,6'), 
                 SimulationParameter(key='orderParameterUpperLimitList',val='0:-25'),
                 SimulationParameter(key='maxSteps',val='1000000000'),
                 SimulationParameter(key='maxWorkUnitSteps',val='100000000')]
    # production reaction rates get swept through in x...
    inputTupssX = [[ReactionRateConstant(reactionID=4, rateConstant=tick), ReactionRateConstant(reactionID=5, rateConstant=tick), ReactionRateConstant(reactionID=11, rateConstant=tick), ReactionRateConstant(reactionID=12, rateConstant=tick)] for tick in xTicks]
    # ...and degradation reaction rates get swept through in y
    inputTupssY = [[ReactionRateConstant(reactionID=6, rateConstant=tick), ReactionRateConstant(reactionID=13, rateConstant=tick)] for tick in yTicks]
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='production%.5f', labelVals=xTicks)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='degradation%.5f', labelVals=yTicks)

    sweep_dict = {'cpu_count': 16,
                  'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 1e3,
                  'lm_sampling_time': 1e10,
                  'queue': queue,
                  'replicateRange': (1,10000),
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_basin_A_initial_flux'),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
