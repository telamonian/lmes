#!/usr/bin/env python

import numpy as np

from lma.src.script.lmFile import Basin,SimulationParameter,Tiling
from lma.regression.regression import FFluxRegressionParser
from lma.regression.models.genetic_toggle_switch.gtsRegression import GTSRegression

class FFluxGTSRegression(GTSRegression):
    helpMessage = 'script to test out a complete Forward Flux Lattice Microbes run'
    parserType = FFluxRegressionParser

    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick_test']:
            return {'batchSize': 1,
                    "errorGoal": .99,
                    "errorGoalConfidence": .01,
                    "pilotStageCount": 1,
                    "productionStageCountMinimum": 1,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": False,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    'phaseZeroSamplingMultiplier': 1e5,
                    'ffluxMinimizeCost': True,
                    'writeInitialTrajectoryState': False,
                    'writeFinalTrajectoryState': False,
                    'writeInterval': None,
                    'writeLimitTracking': True,}
        else:
            return {'batchSize': 1,
                    "errorGoal": .05,
                    "errorGoalConfidence": .95,
                    "pilotStageCount": 1e3,
                    "productionStageCountMinimum": 1e3,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": True,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    'phaseZeroSamplingMultiplier': 10,
                    'ffluxMinimizeCost': False,
                    'writeInterval': 1e20,
                    'writeLimitTracking': False,}

    def _buildInput(self, lmInput):
        # call the parent class method
        lmInput = super(FFluxGTSRegression, self)._buildInput(lmInput=lmInput)

        tilings = [
            Tiling(id=0,
                   orderParameterID=0,
                   type=0,
                   edges=np.linspace(-27,27,13))]

        basins = [
            Basin(tilingID=0,
                  speciesCountArray=np.array(([4,16,1,0,0,0,0],), dtype=np.dtype('uint32')))]
                                              #[0,0,0,4,16,1,0]), dtype=np.dtype('uint32')))]

        if self.parser['extra_input']:
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

        lmInput.AddTilings(tilings=tilings, currentTilingID=0)
        lmInput.AddBasins(basins=basins)

        return lmInput

    def _buildSimulationParameters(self, lmInput):
        simParamKeysToUnset = {'maxSteps', 'maxTime', 'writeInterval'}

        defaultSimParamDict = self.buildDefaultSimulationParameterDict()
        userSimParamDict = self.parser.simParamDict
        for key,val in defaultSimParamDict.items():
            if val is None or val=='None':
                simParamKeysToUnset.update((key,))
            elif key not in userSimParamDict:
                userSimParamDict[key] = defaultSimParamDict[key]

        simParamsToUnset = [SimulationParameter(key=key, val=None) for key in simParamKeysToUnset if key not in userSimParamDict]
        lmInput.UnsetSimulationParameters(simParams=simParamsToUnset)

        return userSimParamDict

if __name__=='__main__':
    regression = FFluxGTSRegression()
    regression.main()

# after this script sets up genetic_toggle_switch.lm, the simulation can be rerun directly with:
# ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "genetic_toggle_switch.lm" -intout
