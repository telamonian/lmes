#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.self_regulated_gene.srgRegression import FFluxSRGRegression

class FFluxSRGRegressionLMES(FFluxSRGRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
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
                    # 'phaseZeroSamplingMultiplier': 1e2,
                    'ffluxMinimizeCost': True,
                    'writeInitialTrajectoryState': False,
                    'writeFinalTrajectoryState': False,
                    'writeInterval': None,
                    'writeLimitTracking': True,
                    'maxWorkUnitSteps': 1e8,}
        else:
            return {'batchSize': 100,
                    "errorGoal": .01,
                    "errorGoalConfidence": .95,
                    "pilotStageCount": 1e3,
                    "productionStageCountMinimum": 1e3,
                    "ffluxPilotOutput": True,
                    "ffluxPhaseOutput": False,
                    "ffluxStageOutputRaw": True,
                    "ffluxStageOutputSummary": True,
                    # 'phaseZeroSamplingMultiplier': 1,
                    'ffluxMinimizeCost': True,
                    'writeInterval': None,
                    'writeLimitTracking': False,}

if __name__=='__main__':
    regression = FFluxSRGRegressionLMES()
    regression.main()
