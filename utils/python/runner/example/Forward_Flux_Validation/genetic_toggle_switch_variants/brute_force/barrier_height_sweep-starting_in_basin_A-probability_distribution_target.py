#!/usr/bin/env python

import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

#### USER DEFINED VARIABLES ####
# host = 'kirin'
# lm_bin = '/home/erober32/usr/bin/lmes'      #'/home/cklein13/git/lm/build/lmes'
# remote_home_directory = '/home/cklein13'
# jobTypeName = 'sge'
# user_id = 'cklein13'
# pass_exe = None
# user_mail = None

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
    simParams = [SimulationParameter(key='crossingsPerPhase',val=str(int(1e5))),
                 SimulationParameter(key='maxPhaseZeroTime',val=str(int(1e7)))]
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
    return [iSCs, iSCBs] + ops + tilings + simParams

if __name__=='__main__':
    thetaTicks = LogTicks(1,-1,base=10,resolution=1)
    
    sweepTups = []
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

    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e15))),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e8)))]

    sweep_dict = {'cpu_count': 720,
#                   'diagonal': True,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 'auto',    #{'rate':'auto', 'weight':.1}, #1e3
                  'lm_sampling_time': 'auto',
                'pass_exe': pass_exe,
                  'replicateRange': (1,1000),
                  'rootPath': PathJoin(remote_home_directory, 'forward_flux_validation/gts_-_bf_-_basin_A_-_theta'),
                  'sweepTups': sweepTups,
                  'jobTypeName': jobTypeName,
                  'user_id': user_id,
                'user_mail': user_mail}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
