#!/usr/bin/env python

import os
import shutil
import sys
import shlex, subprocess

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python', 'runner'))
from lmFile import Input,InitialSpeciesCounts,OrderParameter,ReactionRateConstant,SimulationParameter

path = sys.argv[1]
fPathsToRemove = ['biphasic_switch.lm', 'biphasic_switch.sfile']
for fPath in fPathsToRemove:
    try:
        os.remove(fPath)
    except OSError:
        pass
shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')
replicateInput = Input('biphasic_switch.lm')

iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])

op = OrderParameter(type=0,
                    id=0,
                    speciesIDs=[0,1,2,3,4,5],
                    speciesCoefficients=[-1,-2,-2,1,2,2])

theta = 10
reactionRateConstants = []
productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta), ReactionRateConstant(reactionID=5, rateConstant=1.0*theta), ReactionRateConstant(reactionID=11, rateConstant=1.0*theta), ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
reactionRateConstants+=productionConstants
degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta), ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
reactionRateConstants+=degradationConstants

simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
             SimulationParameter(key='maxTime',val=str(int(2e6))),
             SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e6))),
             SimulationParameter(key='writeInterval',val=str(int(1e1)))]

replicateInput.SetInitialSpeciesCounts(iSCs=iSCs)
replicateInput.SetOrderParameters(ops=[op])
replicateInput.SetReactionRateConstants(rRates=reactionRateConstants)
replicateInput.SetSimulationParameters(simParams=simParams)
replicateInput.Close()

raw_args = '-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff sfile -fo biphasic_switch.sfile -f "biphasic_switch.lm"'
args = shlex.split(raw_args)
p = subprocess.Popen([path] + args)
p.wait()

#os.execv(path, ['-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"'])
#os.execl(path, '"-r 1,2,3,4,5,6,7,8,9,10"', '-sl', 'lm::cme::GillespieDSolver', '-cr', '1', '-gr', '1/4', '-ff', 'hdf5', '-f', 'biphasic_switch.lm')

#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff sfile -fo biphasic_switch.sfile -f "biphasic_switch.lm"
