#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK

from lma.src.script.lmFile import *
from lma.regression.regression import ReplicateRegressionParser
from lma.regression.models.self_regulated_gene.srgRegression import SRGRegression, SRGRegressionParserMixin

class ReplicateSRGRegression(SRGRegression):
    helpMessage = 'script to test out a complete Replicate Lattice Microbes run'
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

    # def _buildSimulationParameters(self):
    #     defaultSimParamDict = self.buildDefaultSimulationParameterDict()
    #     userSimParamDict = self.parser.simParamDict
    #     for key in defaultSimParamDict.keys():
    #         if key not in userSimParamDict:
    #             userSimParamDict[key] = defaultSimParamDict[key]
    #
    #     if 'fptTrackingList' in userSimParamDict:
    #         if not userSimParamDict['fptTrackingList']: userSimParamDict['fptTrackingList'] = [0]
    #
    #
    #         # limitTup = SpeciesLimit(**self.limitDict['a'])
    #         lmInput.SetLimit(limit=Limit(**self.limitDict['species_a']))
    #
    #     if 'fptOrderParameterTrackingList' in userSimParamDict:
    #         if not userSimParamDict['fptOrderParameterTrackingList']: userSimParamDict['fptOrderParameterTrackingList'] = [0]
    #         fptTracking = FirstPassageTimeTracking(valID==userSimParamDict.pop('fptOrderParameterTrackingList'))
    #
    #         # limitTup = OrderParameterLimit(**self.limitDict['a'])
    #         lmInput.SetLimit(limit=Limit(**self.limitDict['oparam_a']))
    #
    #     return userSimParamDict

if __name__=='__main__':
    regression = ReplicateSRGRegression()
    regression.main()

# after this script sets up self_regulated_gene.lm, the simulation can be rerun directly with any of the following lines:
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "self_regulated_gene.lm"
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff sfile -fo self_regulated_gene.sfile -f "self_regulated_gene.lm"
