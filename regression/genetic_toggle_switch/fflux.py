#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.genetic_toggle_switch.gtsRegression import FFluxGTSRegression

class FFluxGTSRegressionLMES(FFluxGTSRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
    # def buildBasins(self):
    #     self.parser['both-basins'] = True
    #     return super(FFluxGTSRegressionLMES, self).buildBasins()

    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick']:
            return {
                "errorGoal": .5,
                "errorGoalConfidence": .5,
                "ffluxPilotOutput": False,
                "ffluxPhaseOutput": False,
                "ffluxStageOutputRaw": True,
                "ffluxStageOutputSummary": True,
                'pilotStageCount': 1e2,
                'productionStageCountMinimum': 1e2,
                # 'writeInterval': 4.0,
                # 'writeInitialTrajectoryState': True,
                # 'writeFinalTrajectoryState': True,
                'writeLimitTracking': True,
            }
        else:
            # return {
            #     "errorGoal": .99,
            #     "errorGoalConfidence": .01,
            #     "ffluxPilotOutput": False,
            #     "ffluxPhaseOutput": False,
            #     "ffluxStageOutputRaw": True,
            #     "ffluxStageOutputSummary": True,
            #     'ffluxMinimizeCost': False,
            #     'stepsPerWorkUnitPart': 4e5, #75000, #40000, #37000,
            #     "pilotStageCount": 1,
            #     'phaseZeroSamplingMultiplier': 5000,
            #     "productionStageCountMinimum": 1,
            #     'writeInitialTrajectoryState': False,
            #     'writeFinalTrajectoryState': False,
            #     'writeInterval': None,
            #     'writeLimitTracking': True,
            # }

            return {
                "errorGoal": .10,
                "errorGoalConfidence": .95,
                "ffluxPilotOutput": False,
                "ffluxPhaseOutput": False,
                "ffluxStageOutputRaw": True,
                "ffluxStageOutputSummary": True,
                'pilotStageCount': 1e3,
                'writeInterval': 4.0,
                # 'writeInitialTrajectoryState': True,
                # 'writeFinalTrajectoryState': True,
                # 'writeLimitTracking': True,
            }

if __name__=='__main__':
    regression = FFluxGTSRegressionLMES()
    regression.main()
