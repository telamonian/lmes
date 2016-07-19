#!/usr/bin/env python
from __future__ import absolute_import

import os,sys
import numpy as np
import sys

from lma.src.script.lmFile import Input,Basin,Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling
from regression import Regression
from replicate import ReplicateRegression

class FFluxRegression(ReplicateRegression):
    defaultLMArgs = Regression.defaultLMArgs + ['-fflux', '-intout']
    helpMessage = 'script to test out a complete Forward Flux Lattice Microbes run'

    def BuildInput(self, **kwargs):
        # call the parent class method
        super(FFluxRegression, self).BuildInput(**kwargs)

        if kwargs['quick_test']:
            defaultSimulationParameters = {"precisionGoal": .20,
                                           "precisionGoalConfidence": .99,
                                           "pilotStageCount": 1e3,
                                           "ffluxPilotOutput": True,
                                           "ffluxPhaseOutput": False,
                                           "ffluxStageOutputRaw": True,
                                           "ffluxStageOutputSummary": True}
        else:
            defaultSimulationParameters = {"precisionGoal": .05,
                                           "precisionGoalConfidence": .95,
                                           "pilotStageCount": 1e3,
                                           "ffluxPilotOutput": False,
                                           "ffluxPhaseOutput": False,
                                           "ffluxStageOutputRaw": False,
                                           "ffluxStageOutputSummary": True}

        ffluxInput = Input('biphasic_switch.lm')

        simParams = [SimulationParameter(key=key, val=str(kwargs.get(key, defaultValue))) for key,defaultValue in defaultSimulationParameters.items()]

        tilings = [
            Tiling(id=0,
                   orderParameterID=0,
                   type=0,
                   edges=np.linspace(-27,27,13))]

        basins = [
            Basin(tilingID=0,
                  speciesCountArray=np.array(([4,16,1,0,0,0,0],), dtype=np.dtype('uint32')))]
                                              #[0,0,0,4,16,1,0]), dtype=np.dtype('uint32')))]

        if kwargs['extra_input']:
            tilings+=[
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
                Tiling(id=7,
                       orderParameterID=0,
                       type=0,
                       edges=np.linspace(-25,25,11))]

            tileCounts = [2] + list(range(4,21))[::4]
            for i in tileCounts:
                id = 100 + i
                numEdges = i+1
                tilings.append(Tiling(id=id,
                                      orderParameterID=0,
                                      type=0,
                                      edges=np.linspace(-27, 27, numEdges)))

        ffluxInput.AddTilings(tilings=tilings, currentTilingID=0)
        ffluxInput.AddBasins(basins=basins)
        ffluxInput.SetSimulationParameters(simParams=simParams)
        ffluxInput.Close()

if __name__=='__main__':
    regression = FFluxRegression()
    regression.Main()

# after this script sets up biphasic_switch.lm, the simulation can be rerun directly with:
# ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm" -intout
