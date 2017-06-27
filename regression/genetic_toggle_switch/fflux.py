#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.genetic_toggle_switch.gtsRegression import FFluxGTSRegression

class FFluxGTSRegressionLMES(FFluxGTSRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick_test']:
            return {
                'batchSize': 100,
                "errorGoal": .5,
                "errorGoalConfidence": .5,
                "ffluxPilotOutput": True,
                "ffluxPhaseOutput": False,
                "ffluxStageOutputRaw": True,
                "ffluxStageOutputSummary": True,
                'ffluxMinimizeCost': True,
                'maxWorkUnitSteps': 1e8,
                # 'phaseZeroSamplingMultiplier': 1,
                "pilotStageCount": 100,
                "productionStageCountMinimum": 1,
                'writeInitialTrajectoryState': False,
                'writeFinalTrajectoryState': False,
                'writeInterval': None,
                'writeLimitTracking': False,
            }
        else:
            # return {
            #     'batchSize': 100,
            #     "errorGoal": .99,
            #     "errorGoalConfidence": .01,
            #     "ffluxPilotOutput": False,
            #     "ffluxPhaseOutput": False,
            #     "ffluxStageOutputRaw": True,
            #     "ffluxStageOutputSummary": True,
            #     'ffluxMinimizeCost': False,
            #     'maxWorkUnitSteps': 4e5, #75000, #40000, #37000,
            #     "pilotStageCount": 1,
            #     'phaseZeroSamplingMultiplier': 5000,
            #     "productionStageCountMinimum": 1,
            #     'writeInitialTrajectoryState': False,
            #     'writeFinalTrajectoryState': False,
            #     'writeInterval': None,
            #     'writeLimitTracking': True,
            # }

            return {
                'batchSize': 100,
                "errorGoal": .10,
                "errorGoalConfidence": .95,
                "ffluxPilotOutput": False,
                "ffluxPhaseOutput": True,
                "ffluxStageOutputRaw": True,
                "ffluxStageOutputSummary": True,
                'ffluxMinimizeCost': False,
                'maxWorkUnitSteps': 1e8,
                "pilotStageCount": 1e4,
                # 'phaseZeroSamplingMultiplier': 100,
                "productionStageCountMinimum": 1e3,
                'writeInitialTrajectoryState': False,
                'writeFinalTrajectoryState': False,
                'writeInterval': None, #-1.0,
                'writeLimitTracking': False,
            }

if __name__=='__main__':
    regression = FFluxGTSRegressionLMES()
    regression.main()
