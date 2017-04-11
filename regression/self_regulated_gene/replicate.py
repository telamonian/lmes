#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.models.self_regulated_gene.srgRegression import ReplicateSRGRegression

class ReplicateSRGRegressionLMES(ReplicateSRGRegression):
    """subclass that allows for easy overriding of various parameters at point of testing
    """
    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick_test']:
            return {'maxSteps': 1e10,
                    'maxTime': 1e4,
                    'maxWorkUnitSteps': 1e8,
                    'writeInterval': 1e0,
                    #'orderParameterWriteInterval': 1e0,
                    }
        else:
            return {'maxSteps': 1e10,
                    'maxTime': 1e6,
                    'maxWorkUnitSteps': 1e8,
                    'writeInterval': 1e1,
                    # 'orderParameterWriteInterval': 1e1,
                    }

if __name__=='__main__':
    regression = ReplicateSRGRegressionLMES()
    regression.main()