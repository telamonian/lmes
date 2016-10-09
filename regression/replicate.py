#!/usr/bin/env python
from __future__ import absolute_import

import os,sys
import shutil
from six import print_
import sys

from lma.src.script.lmFile import Input,Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling
from regression import Regression

class ReplicateRegression(Regression):
    defaultLMArgs = ['-r', '1-100'] + Regression.defaultLMArgs
    helpMessage = 'script to test out a complete Replicate Lattice Microbes run'

    def _BuildInput(self, **kwargs):
        if kwargs['quick_test']:
            defaultSimulationParameters = {'maxSteps': str(int(1e10)),
                                           'maxTime': str(int(1e1)),
                                           # 'maxWorkUnitSteps': str(int(1e8)),
                                           'writeInterval': str(int(1e0))
                                           # 'orderParameterWriteInterval': str(int(1e0))}
                                           }
            theta = float(kwargs.get('theta', 10))
        else:
            defaultSimulationParameters = {'maxSteps': str(int(1e10)),
                                           'maxTime': str(int(1e4)),
                                           'maxWorkUnitSteps': str(int(1e4)),
                                           'writeInterval': str(int(1e1)),
                                           # 'orderParameterWriteInterval': str(int(1e1))}
                                           }
            theta = kwargs.get('theta', 1)

        if 'orderParameterWriteInterval' in kwargs: defaultSimulationParameters['orderParameterWriteInterval'] = kwargs['orderParameterWriteInterval']
        simParams = [SimulationParameter(key=key, val=kwargs.get(key, defaultSimulationParameters[key])) for key in defaultSimulationParameters.keys()]

        replicateInput = Input('biphasic_switch.lm')

        iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])

        ops = [
            OrderParameter(type=0,
                           id=0,
                           speciesIDs=[0,1,2,3,4,5],
                           # speciesCoefficients=[-1,-8,-8,1,8,8])]
                           speciesCoefficients=[-1,-2,-2,1,2,2])]

        if kwargs['extra_input']:
            ops+=[
                OrderParameter(type=0,
                               id=1,
                               speciesIDs=[0,1,2],
                               speciesCoefficients=[1,2,2]),
                OrderParameter(type=0,
                               id=2,
                               speciesIDs=[3,4,5],
                               speciesCoefficients=[1,2,2])]

        psi = 1.0
        reactionDict = {'dimerization_a':      {'reactionID': 0, 'rateConstant': 5.0},
                        'dedimerization_a':    {'reactionID': 1, 'rateConstant': 5.0},
                        'opbinding_a':         {'reactionID': 2, 'rateConstant': 5.0*psi},
                        'opunbinding_a':       {'reactionID': 3, 'rateConstant': 1.0*psi},
                        'boundproduction_a':   {'reactionID': 4, 'rateConstant': 1.0*theta},
                        'unboundproduction_a': {'reactionID': 5, 'rateConstant': 1.0*theta},
                        'degradation_a':       {'reactionID': 6, 'rateConstant': 0.25*theta},
                        'dimerization_b':      {'reactionID': 7, 'rateConstant': 5.0},
                        'dedimerization_b':    {'reactionID': 8, 'rateConstant': 5.0},
                        'opbinding_b':         {'reactionID': 9, 'rateConstant': 5.0*psi},
                        'opunbinding_b':       {'reactionID': 10, 'rateConstant': 1.0*psi},
                        'boundproduction_b':   {'reactionID': 11, 'rateConstant': 1.0*theta},
                        'unboundproduction_b': {'reactionID': 12, 'rateConstant': 1.0*theta},
                        'degradation_b':       {'reactionID': 13, 'rateConstant': 0.25*theta}}

        # productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta),
        #                        ReactionRateConstant(reactionID=5, rateConstant=1.0*theta),
        #                        ReactionRateConstant(reactionID=11, rateConstant=1.0*theta),
        #                        ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
        # degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta),
        #                         ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
        # reactionRateConstants = productionConstants + degradationConstants

        reactionRateConstants = [ReactionRateConstant(**d) for d in reactionDict.values()]

        replicateInput.SetInitialSpeciesCounts(iSCs=iSCs)
        replicateInput.SetOrderParameters(ops=ops)
        replicateInput.SetReactionRateConstants(rRates=reactionRateConstants)
        replicateInput.SetSimulationParameters(simParams=simParams)
        if kwargs['firstPassageTimeSpecies']:
            replicateInput.SetFirstPassageTimeTracking(fptTrackedSpecies=[0,1,2,3,4,5,6])

        if kwargs['firstPassageTimeOrderParameters']:
            replicateInput.SetFirstPassageTimeTracking(fptTrackedOrderParameters=[0])

        replicateInput.Close()

if __name__=='__main__':
    regression = ReplicateRegression()
    regression.Main()

# after this script sets up biphasic_switch.lm, the simulation can be rerun directly with any of the following lines:
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"
#../build/lmes -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff sfile -fo biphasic_switch.sfile -f "biphasic_switch.lm"
