#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

import numpy as np

from lma.src.script.lmFile import Basin,SimulationParameter,Tiling
from lma.regression.regression import FFluxRegressionParser
from lma.regression.models.self_regulated_gene.srgRegression import SRGRegression, SRGRegressionParserMixin

class FFluxSRGRegression(SRGRegression):
    helpMessage = 'script to test out a complete Forward Flux Lattice Microbes run'
    # dynamically create the parser type from the normal fflux simulation parser and the self regulated gene parser mixin
    parserType = type('FFluxSRGRegressionParser', (SRGRegressionParserMixin, FFluxRegressionParser), {})

    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick_test']:
            return {'batchSize': 1,
                    "errorGoal": .99,
                    "errorGoalConfidence": .01,
                    "pilotStageCount": 1,
                    "productionStageCountMinimum": 1,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": True,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    'phaseZeroSamplingMultiplier': 1e2,
                    'ffluxMinimizeCost': True,
                    'writeInitialTrajectoryState': False,
                    'writeFinalTrajectoryState': False,
                    'writeInterval': None,
                    'writeLimitTracking': True,
                    'maxWorkUnitSteps': 1e5,}
        else:
            return {'batchSize': 1,
                    "errorGoal": .01,
                    "errorGoalConfidence": .95,
                    "pilotStageCount": 1e3,
                    "productionStageCountMinimum": 1e3,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": False,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    'phaseZeroSamplingMultiplier': 1,
                    'ffluxMinimizeCost': False,
                    'writeInterval': None,
                    'writeLimitTracking': False,}

    def _buildInput(self, lmInput):
        # call the parent class method
        lmInput = super(FFluxSRGRegression, self)._buildInput(lmInput=lmInput)

        tilings = [
            Tiling(id=0,
                   orderParameterID=0,
                   type=0,
                   edges=np.linspace(20,128,13))]

        basinArray = np.array([13]*self.parser['basinReplicates'], dtype=np.dtype('uint32')).reshape(self.parser['basinReplicates'], -1)
        basins = [
            Basin(tilingID=0,
                  speciesCountArray=basinArray)]

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
    regression = FFluxSRGRegression()
    regression.main()

# after this script sets up genetic_toggle_switch.lm, the simulation can be rerun directly with:
# ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "genetic_toggle_switch.lm" -intout
