#!/usr/bin/env python

import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
crossingsPerPhase = str(int(1e4))
host = 'xanthus'
lm_bin = '/home/cklein13/git/lm/build_cuda/lmes'
local_home_directory = thisScriptsPath
maxPhaseZeroTime = str(int(1e4))
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
    return [iSCs, iSCBs, op] + simParams

if __name__=='__main__':
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    theta = 100
    reactionRateConstants = []
    productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta), ReactionRateConstant(reactionID=5, rateConstant=1.0*theta), ReactionRateConstant(reactionID=11, rateConstant=1.0*theta), ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
    reactionRateConstants+=productionConstants
    degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta), ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
    reactionRateConstants+=degradationConstants
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15)))]
    
    # building up the sweep along the tiling truncations
    edges = np.linspace(-25,25,13)
    tilings = []
    for i in range(1,len(edges) + 1):
        tiling = Tiling(id=0,
                 orderParameterID=0,
                 type=0,
                 edges=edges[0:i])
        tilings.append(tiling)
    inputTupss = [[tiling] for tiling in tilings] 
    sweepTup = SweepTup(inputTupss=inputTupss, label='interfacesTruncatedAt%0d', labelVals=range(len(edges)))

    sweep_dict = {'cpu_count': 16,
                  'host': host,
                  'inputTupsDefault': GetFFluxInputTups() + reactionRateConstants + simParams,
                  'lmArgsIntout': True,
                  'lmArgsGpusPerReplicate': '1/4',
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 1e3, #'auto',
                  'lm_sampling_time': 1e10,
                  'queue': queue,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_fflux_truncated_tiling_theta%02.1f' % theta),
                  'sweepTups': [sweepTup],
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
