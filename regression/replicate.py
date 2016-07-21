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

    def BuildInput(self, **kwargs):
        if kwargs['quick_test']:
            defaultSimulationParameters = {'maxSteps': str(int(1e10)),
                                           'maxTime': str(int(1e1)),
                                           'maxWorkUnitSteps': str(int(1e8)),
                                           'writeInterval': str(int(1e0))}
                                           # 'orderParameterWriteInterval': str(int(1e0))}
            theta = 10
        else:
            defaultSimulationParameters = {'maxSteps': str(int(1e10)),
                                           'maxTime': str(int(1e4)),
                                           'maxWorkUnitSteps': str(int(1e8)),
                                           'writeInterval': str(int(1e1))}
                                           # 'orderParameterWriteInterval': str(int(1e1))}
            theta = kwargs['theta']

        if 'orderParameterWriteInterval' in kwargs: defaultSimulationParameters['orderParameterWriteInterval'] = kwargs['orderParameterWriteInterval']

        try:
            os.remove('biphasic_switch.lm')
        except OSError:
            pass
        shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')

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

        simParams = [SimulationParameter(key=key, val=kwargs.get(key, defaultSimulationParameters[key])) for key in defaultSimulationParameters.keys()]

        productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta),
                               ReactionRateConstant(reactionID=5, rateConstant=1.0*theta),
                               ReactionRateConstant(reactionID=11, rateConstant=1.0*theta),
                               ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
        degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta),
                                ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
        reactionRateConstants = productionConstants + degradationConstants

        replicateInput.SetInitialSpeciesCounts(iSCs=iSCs)
        replicateInput.SetOrderParameters(ops=ops)
        replicateInput.SetReactionRateConstants(rRates=reactionRateConstants)
        replicateInput.SetSimulationParameters(simParams=simParams)
        print_(kwargs)
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
