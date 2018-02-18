#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.self_regulating_gene.srgRegression import FFluxSRGRegression

class FFluxSRGRegressionLMES(FFluxSRGRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
    def buildBasins(self):
        self.parser['both-basins'] = True
        return super(FFluxSRGRegressionLMES, self).buildBasins()

    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick']:
            return {
                'ffluxPilotOutput': True,
                'ffluxPhaseOutput': True,
                'ffluxStageOutputRaw': True,
                'ffluxStageOutputSummary': True,
                'errorGoal': .9,
                'errorGoalConfidence': .01,
                'pilotStageCount': 10,
                'productionStageCountMinimum': 10,
                'writeInterval': 10.0,
                
                # 'batchSize': 1,
                # 'phaseZeroSamplingMultiplier': 1e2,
                # 'ffluxMinimizeCost': True,
                # 'writeInitialTrajectoryState': False,
                # 'writeFinalTrajectoryState': False,
                # 'writeInterval': None,
                # 'writeLimitTracking': True,
                # 'stepsPerWorkUnitPart': 1e8,
            }
        else:
            return {
                'errorGoal': .3,
                'pilotStageCount': 1e2,
                'productionStageCountMinimum': 1e2,
                'writeInterval': 1.0,
                
                # 'batchSize': 100,
                # 'errorGoalConfidence': .95,
                # 'ffluxPilotOutput': True,
                # 'ffluxPhaseOutput': False,
                # 'ffluxStageOutputRaw': True,
                # 'ffluxStageOutputSummary': True,
                # 'phaseZeroSamplingMultiplier': 1,
                # 'ffluxMinimizeCost': True,
                # 'writeLimitTracking': False,
            }

if __name__=='__main__':
    regression = FFluxSRGRegressionLMES()
    regression.main()
