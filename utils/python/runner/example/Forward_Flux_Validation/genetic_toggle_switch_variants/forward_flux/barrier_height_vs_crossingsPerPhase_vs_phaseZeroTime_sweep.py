#!/usr/bin/env python
import numpy as np; np.set_printoptions(precision=1, threshold=1e6, linewidth=1e6)
import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
host = 'kirin'
lm_bin = '/home/cklein13/git/lm/build/lmes'
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
    tilings = [
        Tiling(id=0,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-27,27,13),
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
        Tiling(id=3,
               orderParameterID=0,
               type=0,
               edges=np.arange(-100,100)),
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
    return [iSCs, iSCBs] + ops + tilings
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
    cppTicks = LogTicks(2,4,base=10,resolution=0)
    pztTicks = LogTicks(2,6,base=10,resolution=-.5)
    thetaTicks = LogTicks(-1,0,base=10,resolution=4)[:-1]
    
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15)))]
    
    sweepTups = []
    # crossings per phase sweep
    cppInputTups = []
    for crossingPerPhase in cppTicks:
        cppInputTups.append([SimulationParameter(key='crossingsPerPhase',val=crossingPerPhase)])
    sweepTups.append(SweepTup(inputTupss=cppInputTups, label='cpp_%.1e', labelVals=cppTicks))
    
    # phase zero time sweep
    pztInputTups = []
    for phaseZeroTime in pztTicks:
        pztInputTups.append([SimulationParameter(key='maxPhaseZeroTime',val=phaseZeroTime)])
    sweepTups.append(SweepTup(inputTupss=pztInputTups, label='pzt_%.1e', labelVals=pztTicks))
    
    # barrier height sweep
    thetaInputTups = []
    for theta in thetaTicks:
        productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=5, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=11, rateConstant=1.0*theta), 
                               ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
        degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta), 
                                ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
        thetaInputTups.append(productionConstants + degradationConstants)
    sweepTups.append(SweepTup(inputTupss=thetaInputTups, label='theta_%.1e', labelVals=thetaTicks))
    
    sweep_dict = {'cpu_count': 8,
                  #'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lmArgsIntout': True,
#                   'lmArgsGpusPerReplicate': '1/4',
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 'auto',    #{'rate':'auto', 'weight':.1}, #1e3
                  'lm_sampling_time': 1e10,
#                   'queue': queue,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_-_fflux_-_barrier_height_-_crossingsPerPhase_-_phaseZeroTime'),
                  'sweepTups': sweepTups,
                  'type': type,
                  'useForwardFlux': True,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
