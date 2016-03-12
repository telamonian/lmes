#!/usr/bin/env python
from __future__ import absolute_import

import os,sys
import numpy as np
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python', 'runner'))
from lmFile import Input,Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling
from regression import Regression
from replicate import ReplicateRegression

class FFluxRegression(ReplicateRegression):
    defaultLMArgs = Regression.defaultLMArgs + ['-fflux', '-intout']
    helpMessage = 'script to test out a complete Forward Flux Lattice Microbes run'

    def BuildInput(self, **kwargs):
        # call the parent class method
        super(FFluxRegression, self).BuildInput(**kwargs)

        phaseCheckDict = {'maxCrossingsZero': 1e3,      #5e4    #1e5
                          'maxTimeZero': None, #1e4     #5e5    #1e6
                          'maxCrossingsN': 1e3,         #5e4    #1e5
                          'maxTimeN': None}
        phaseCheckDictOverride = {key:val for key,val in ((key, kwargs.pop(key)) for key in ('maxCrossingsZero', 'maxTimeZero', 'maxCrossingsN', 'maxTimeN')) if val is not None}
        phaseCheckDict.update(phaseCheckDictOverride)

        ffluxInput = Input('biphasic_switch.lm')

        iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])

        simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                     SimulationParameter(key='maxTime',val=str(int(1e10))),
                     SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15))),
                     SimulationParameter(key='writeInterval',val='%.10f' % (1.0/(.25*kwargs['theta'])))]

        for key,val in phaseCheckDict.items():
            if val is not None:
                simParams.append(SimulationParameter(key=key, val=str(int(float(val)))))

        tilings = [
            Tiling(id=0,
                   orderParameterID=0,
                   type=0,
                   edges=np.linspace(-27,27,13)),
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

        ffluxInput.AddTilings(tilings=tilings, currentTilingID=0)
        ffluxInput.SetInitialSpeciesCountsBackward(iSCBs=iSCBs)
        ffluxInput.SetSimulationParameters(simParams=simParams)
        ffluxInput.Close()

if __name__=='__main__':
    regression = FFluxRegression()
    regression.Main()

# after this script sets up biphasic_switch.lm, the simulation can be rerun directly with:
# ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm" -intout