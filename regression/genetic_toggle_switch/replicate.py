#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.regression import ReplicateRegressionParser
from lma.regression.models.genetic_toggle_switch.gtsRegression import GTSRegression

class ReplicateGTSRegression(GTSRegression):
    helpMessage = 'script to test out a complete Replicate Lattice Microbes run with the Genetic Toggle Switch model'
    parserType = ReplicateRegressionParser
    
    def _buildDefaultSimulationParameterDict(self):
        if self.parser['quick_test']:
            return {'maxSteps': 1e10,
                    'maxTime': 1e1,
                    'maxWorkUnitSteps': 1e8,
                    'writeInterval': 1e0,
                    # 'orderParameterWriteInterval': 1e0}
                    }
        else:
            return {'maxSteps': 1e10,
                    'maxTime': 1e4,
                    'maxWorkUnitSteps': 1e8,
                    'writeInterval': 1e1,
                    # 'orderParameterWriteInterval': 1e1}
                    }

if __name__=='__main__':
    regression = ReplicateGTSRegression()
    regression.main()