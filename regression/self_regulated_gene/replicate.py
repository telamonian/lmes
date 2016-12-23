#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.regression import ReplicateRegressionParser
from lma.regression.models.self_regulated_gene.srgRegression import SRGRegression, SRGRegressionParserMixin

class ReplicateSRGRegression(SRGRegression):
    helpMessage = 'script to test out a complete Replicate Lattice Microbes run with the Self Regulated Gene model'
    # dynamically create the parser type from the normal replicate simulation parser and the self regulated gene parser mixin
    parserType = type('ReplicateSRGRegressionParser', (SRGRegressionParserMixin, ReplicateRegressionParser), {})
    
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
    regression = ReplicateSRGRegression()
    regression.main()