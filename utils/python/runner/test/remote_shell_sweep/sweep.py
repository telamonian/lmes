#!/usr/bin/env python

import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

host = 'kirin'
lm_bin = '/home/cklein13/git/lm/build/lm'
local_home_directory = thisScriptsPath
remote_home_directory = '/home/cklein13'
type = 'shell'
user_id = 'cklein13'

if __name__ == "__main__" and __package__ is None:
    __package__ = "runner.test.remote_sge"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    script_dir_three_up = os.path.split(os.path.split(os.path.split(script_dir)[0])[0])[0]
    sys.path.append(script_dir_three_up)
    import runner.test

from ...helper import *
from ...job import JobSGE
from ...runner import Runner
from ...sweep import InitialSpeciesCounts, InitialSpeciesCountsBackward, OrderParameter, ReactionRateConstant, SimulationParameter, Tiling, Sweep, SweepTup

def GetFFluxInputTups():
    iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])
    iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])
    op = OrderParameter(type=0,
                        id=0,
                        speciesIDs=[0,1,2,3,4,5],
                        speciesCoefficients=[-1,-2,-2,1,2,2])
    simParams = [SimulationParameter(key='crossingsPerPhase',val='100'),
                 SimulationParameter(key='maxPhaseZeroTime',val='10000')]
    tiling = Tiling(id=0,
                    orderParameterID=0,
                    Type=0,
                    edges=np.linspace(-25,25,13))
    return [iSCs, iSCBs, op, tiling] + simParams

if __name__=='__main__':
    xTicks = LogTicks(-2,-2,base=10,resolution=1)
    yTicks = xTicks/4.0
    # these inputTupsDefault get applied to every lm file before any simulations in the sweep
    simParams = [SimulationParameter(key='maxSteps',val='1e10'),
                 SimulationParameter(key='maxWorkUnitSteps',val='10000')]
    # production reaction rates get swept through in x...
    inputTupssX = [[ReactionRateConstant(reactionID=4, rateConstant=tick), ReactionRateConstant(reactionID=5, rateConstant=tick), ReactionRateConstant(reactionID=11, rateConstant=tick), ReactionRateConstant(reactionID=12, rateConstant=tick)] for tick in xTicks]
    # ...and degradation reaction rates get swept through in y
    inputTupssY = [[ReactionRateConstant(reactionID=6, rateConstant=tick), ReactionRateConstant(reactionID=13, rateConstant=tick)] for tick in yTicks]
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='production%.5f', labelVals=xTicks)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='degradation%.5f', labelVals=yTicks)
    
    sweep_dict = {'cpu_count': 2,
                  'host': host,
                  'inputTupsDefault': simParams + GetFFluxInputTups(),
                  'lm_bin': lm_bin,
                  'lm_file_path': 'biphasic_switch.lm',
                  'lm_sampling_rate': 'auto',
                  'lm_sampling_time': 'auto',
                  'replicateRange': (1,100),
                  'rootPath': PathJoin(remote_home_directory, 'test/sweep'),
                  'sweepTupX': sweepTupX,
                  'sweepTupY': sweepTupY,
                  'type': type,
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
