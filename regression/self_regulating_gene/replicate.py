#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.self_regulating_gene.srgRegression import ReplicateSRGRegression

class ReplicateSRGRegressionLMES(ReplicateSRGRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick']:
            return {
                'maxTime': 1e4,
                'stepsPerWorkUnitPart': 1e6,
                'writeInterval': 1e0,
                #'orderParameterWriteInterval': 1e0,

                # obsolete parameters
                # 'maxSteps': 1e10,
            }
        else:
            return {
                'maxTime': 1e6,
                'stepsPerWorkUnitPart': 1e8,
                'writeInterval': 1e1,
                # 'orderParameterWriteInterval': 1e1,

                # obsolete parameters
                # 'maxSteps': 1e10,
            }

if __name__=='__main__':
    regression = ReplicateSRGRegressionLMES()
    regression.main()