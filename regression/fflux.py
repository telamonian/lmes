#!/usr/bin/env python

import os
import numpy as np
import shutil
import sys
import shlex, subprocess

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python', 'runner'))
from lmFile import Input,Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling

path = sys.argv[1]
try:
    fullLength = sys.argv[2]
    if fullLength=='True' or fullLength=='true':
        fullLength = True
except IndexError:
    fullLength = False
if fullLength==True:
    crossingsPerPhase = str(int(1e4))
    maxPhaseZeroTime = str(int(1e6))
else:
    crossingsPerPhase = str(int(1e2))
    maxPhaseZeroTime = str(int(1e4))

try:
    os.remove('biphasic_switch.lm')
except OSError:
    pass
shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')

ffluxInput = Input('biphasic_switch.lm')
iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])
iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])
ops = []
op = OrderParameter(type=0,
                    id=0,
                    speciesIDs=[0,1,2,3,4,5],
                    speciesCoefficients=[-1,-2,-2,1,2,2])
ops.append(op)
op = OrderParameter(type=0,
                    id=1,
                    speciesIDs=[0,1,2],
                    speciesCoefficients=[1,2,2])
ops.append(op)
op = OrderParameter(type=0,
                    id=2,
                    speciesIDs=[3,4,5],
                    speciesCoefficients=[1,2,2])
ops.append(op)
simParams = [SimulationParameter(key='crossingsPerPhase',val=crossingsPerPhase),
             SimulationParameter(key='maxPhaseZeroTime',val=maxPhaseZeroTime),
             SimulationParameter(key='maxSteps',val=str(int(1e10))),
             SimulationParameter(key='maxTime',val='1e5'),
             SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e7))),
             SimulationParameter(key='writeInterval',val='1e3')]
tilings = []
tiling = Tiling(id=0,
                orderParameterID=0,
                type=0,
                edges=np.linspace(-25,25,13))
tilings.append(tiling)
tiling = Tiling(id=1,
                orderParameterID=1,
                type=0,
                edges=np.linspace(-25,25,13))
tilings.append(tiling)
tiling = Tiling(id=2,
                orderParameterID=2,
                type=0,
                edges=np.linspace(-25,25,13))
tilings.append(tiling)
tiling = Tiling(id=199,
                orderParameterID=0,
                type=0,
                edges=np.linspace(-30,30,16))
tilings.append(tiling)
tiling = Tiling(id=7,
                orderParameterID=0,
                type=0,
                edges=np.linspace(-25,25,11))
tilings.append(tiling)
tiling = Tiling(id=27194,
                orderParameterID=0,
                type=0,
                edges=np.linspace(-20,20,5))
tilings.append(tiling)
ffluxInput.AddTilings(tilings=tilings, currentTilingID=0)
ffluxInput.SetInitialSpeciesCounts(iSCs=iSCs)
ffluxInput.SetInitialSpeciesCountsBackward(iSCBs=iSCBs)
ffluxInput.SetOrderParameters(ops=[op])
ffluxInput.SetSimulationParameters(simParams=simParams)
ffluxInput.Close()

raw_args = '-sl lm::cme::GillespieDSolver -cr 1 -gr 0 -ff hdf5 -fflux -f "biphasic_switch.lm"'
args = shlex.split(raw_args)
p = subprocess.Popen([path] + args)
p.wait()

# after this script sets up biphasic_switch.lm, the simulation can be rerun directly with:
# ../build/lm -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm"
