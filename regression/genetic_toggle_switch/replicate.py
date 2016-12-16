#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.regression.regression import ReplicateRegressionParser
from lma.regression.models.genetic_toggle_switch.gtsRegression import GTSRegression

class ReplicateGTSRegression(GTSRegression):
    helpMessage = 'script to test out a complete Replicate Lattice Microbes run'
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


    def _buildSimulationParameters(self):
        defaultSimParamDict = self.buildDefaultSimulationParameterDict()
        userSimParamDict = self.parser.simParamDict
        for key in defaultSimParamDict.keys():
            if key not in userSimParamDict:
                userSimParamDict[key] = defaultSimParamDict[key]

        if 'firstPassageTimeSpecies' in userSimParamDict:
            if not userSimParamDict['firstPassageTimeSpecies']:
                userSimParamDict['firstPassageTimeSpecies'] = [0,1,2,3,4,5,6]

            if 'fptLimitDict' in userSimParamDict:
                userSimParamDict['fptLimitDict'] += [self.limitDict['species_a']]

        if 'firstPassageTimeOrderParameters' in userSimParamDict:
            if not userSimParamDict['firstPassageTimeOrderParameters']:
                userSimParamDict['firstPassageTimeOrderParameters'] = [0]

            if 'fptLimitDict' in userSimParamDict:
                userSimParamDict['fptLimitDict'] += [self.limitDict['oparam_a']]

        return userSimParamDict

if __name__=='__main__':
    regression = ReplicateGTSRegression()
    regression.main()

# after this script sets up genetic_toggle_switch.lm, the simulation can be rerun directly with any of the following lines:
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "genetic_toggle_switch.lm"
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff sfile -fo genetic_toggle_switch.sfile -f "genetic_toggle_switch.lm"
