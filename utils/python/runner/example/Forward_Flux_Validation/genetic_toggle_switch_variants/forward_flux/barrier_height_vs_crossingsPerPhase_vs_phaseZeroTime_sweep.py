#!/usr/bin/env python
# import numpy as np; np.set_printoptions(precision=1, threshold=1e6, linewidth=1e6)
import os
import sys
from statsmodels.sandbox.tools import cross_val

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
# host = 'xanthus'
# lm_bin = '/home/cklein13/git/lm/build_cuda/lmes'
# queue = 'gpu'
# remote_home_directory = '/home/cklein13'
# jobTypeName = 'sge'
# user_id = 'cklein13'

host = 'gateway2.marcc.jhu.edu'
lm_bin = '/home-1/cklein13@jhu.edu/git/lm/build/lmes'   #'/home-2/erober32@jhu.edu/usr/bin/lmes'
remote_home_directory = '/home-1/cklein13@jhu.edu/work/cklein13'
jobTypeName = 'slurm'
user_id = 'cklein13@jhu.edu'
pass_exe = '/Users/tel/usr/bin/sp_marcc'
user_mail = 'cklein13@jhu.edu'

local_home_directory = thisScriptsPath
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
    cppTicks = [1e4] #LogTicks(2,7,base=10,resolution=0)
    pztTicks = [1e4]   #LogTicks(2,6,base=10,resolution=-.5)
    thetaTicks = [1] #LogTicks(1,-1,base=10,resolution=1)   #LogTicks(-1,1,base=10,resolution=4)
    replicateTicks = [0] #list(range(3))

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

    # replicate sweep
    replicateInputTups = []
    for rep in replicateTicks:
        replicateInputTups.append([])
    sweepTups.append(SweepTup(inputTupss=replicateInputTups, label='rep_%d', labelVals=replicateTicks))

    sweep_dict = {'cpu_count': 24*1,
                  #'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lmArgsIntout': True,
                  'lm_bin': lm_bin,
                  'lm_cores': 8,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 'auto',    #{'rate':'auto', 'weight':.1}, #1e3
                  'lm_sampling_time': 1e10,
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_-_fflux_-_crossingsPerPhase_-_phaseZeroTime_-_theta_-_replicate_-_test'),
                  'sweepTups': sweepTups,
                  'jobTypeName': jobTypeName,
                  'useForwardFlux': True,
                  'user_id': user_id}

    try:
        sweep_dict['queue'] = queue
        if 'gpu' in queue:
            sweep_dict['lmArgsGpusPerReplicate'] = '1/4'
    except NameError:
        pass

    if host=='gateway2.marcc.jhu.edu':
        sweep_dict.update({'pass_exe': pass_exe,
                           'user_mail': user_mail})

    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
