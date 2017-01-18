#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.regression import FFluxRegressionParser
from lma.regression.models.genetic_toggle_switch.gtsRegression import GTSRegression

class FFluxGTSRegression(GTSRegression):
    helpMessage = 'script to test out a complete Forward Flux Lattice Microbes run with the Genetic Toggle Switch model'
    parserType = FFluxRegressionParser

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
                    'phaseZeroSamplingMultiplier': 1,
                    'ffluxMinimizeCost': True,
                    'writeInitialTrajectoryState': True,
                    'writeFinalTrajectoryState': True,
                    'writeInterval': None,
                    'writeLimitTracking': False,
                    'maxWorkUnitSteps': 1e5,}
        else:
            return {'batchSize': 1,
                    "errorGoal": .99,
                    "errorGoalConfidence": .01,
                    "pilotStageCount": 1e4,
                    "productionStageCountMinimum": 1,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": True,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    'phaseZeroSamplingMultiplier': 1,
                    'ffluxMinimizeCost': False,
                    'writeInitialTrajectoryState': True,
                    'writeFinalTrajectoryState': True,
                    'writeInterval': None,
                    'writeLimitTracking': False,}

if __name__=='__main__':
    regression = FFluxGTSRegression()
    regression.main()
