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

theta = 1
productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta), ReactionRateConstant(reactionID=5, rateConstant=1.0*theta), ReactionRateConstant(reactionID=11, rateConstant=1.0*theta), ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta), ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
reactionRateConstants = productionConstants + degradationConstants

simParams = [SimulationParameter(key='crossingsPerPhase',val=crossingsPerPhase),
             SimulationParameter(key='maxPhaseZeroTime',val=maxPhaseZeroTime),
             SimulationParameter(key='maxSteps',val=str(int(1e10))),
             SimulationParameter(key='maxTime',val='1e10'),
             SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15))),
             SimulationParameter(key='writeInterval',val='%.10f' % (1.0/(.25*theta)))]

tilings = [
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

ffluxInput.AddTilings(tilings=tilings, currentTilingID=19)
ffluxInput.SetInitialSpeciesCounts(iSCs=iSCs)
ffluxInput.SetInitialSpeciesCountsBackward(iSCBs=iSCBs)
ffluxInput.SetOrderParameters(ops=ops)
ffluxInput.SetReactionRateConstants(rRates=reactionRateConstants)
ffluxInput.SetSimulationParameters(simParams=simParams)
ffluxInput.Close()

raw_args = '-sl lm::cme::GillespieDSolver -c 5 -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm" -intout'
args = shlex.split(raw_args)
p = subprocess.Popen([path] + args)
p.wait()

# after this script sets up biphasic_switch.lm, the simulation can be rerun directly with:
# ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm" -intout
