#!/usr/bin/env python

import os
import shutil
import sys
import shlex, subprocess

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python', 'runner'))
from lmFile import Input,InitialSpeciesCounts,OrderParameter,SimulationParameter

path = sys.argv[1]
try:
    os.remove('biphasic_switch.lm')
except OSError:
    pass
shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')
opULInput = Input('biphasic_switch.lm')

iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])

op = OrderParameter(type=0,
                    id=0,
                    speciesIDs=[0,1,2,3,4,5],
                    speciesCoefficients=[-1,-2,-2,1,2,2])

opUpperLimit = [SimulationParameter(key='speciesUpperLimitList',val='3:10')]

simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
             SimulationParameter(key='maxTime',val=str(int(1e10))),
             SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e7))),
             SimulationParameter(key='writeInterval',val=str(int(1e3)))]

opULInput.SetInitialSpeciesCounts(iSCs=iSCs)
opULInput.SetOrderParameters(ops=[op])
opULInput.SetSimulationParameters(simParams=simParams + opUpperLimit)
opULInput.Close()

raw_args = '-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"'
args = shlex.split(raw_args)
p = subprocess.Popen([path] + args)
p.wait()

#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"
