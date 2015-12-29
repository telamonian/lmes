#!/usr/bin/env python
from argparse import ArgumentParser
import os
import numpy as np
import re
import shutil
from six import print_
import sys
import shlex, subprocess

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python', 'runner'))
from lmFile import Input,Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling

def _Main(execPath, fullLength, lmArgs, phaseCheckDictOverride, timingLength):
    if fullLength:
        phaseCheckDict = {'maxCrossingsZero': 1e5,
                          'maxTimeZero': None,  #1e6
                          'maxCrossingsN': 1e5,
                          'maxTimeN': None}
    elif timingLength:
        phaseCheckDict = {'maxCrossingsZero': 5e4,
                          'maxTimeZero': None,  #5e5
                          'maxCrossingsN': 5e4,
                          'maxTimeN': None}
    else:
        phaseCheckDict = {'maxCrossingsZero': 1e3,
                          'maxTimeZero': None,  #1e4
                          'maxCrossingsN': 1e3,
                          'maxTimeN': None}
    phaseCheckDict.update(phaseCheckDictOverride)

    try:
        os.remove('biphasic_switch.lm')
    except OSError:
        pass
    shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')

    ffluxInput = Input('biphasic_switch.lm')

    iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])
    iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])

    ops = [
    OrderParameter(type=0,
                   id=0,
                   speciesIDs=[0,1,2,3,4,5],
                   speciesCoefficients=[-1,-2,-2,1,2,2]),
    OrderParameter(type=0,
                   id=1,
                   speciesIDs=[0,1,2],
                   speciesCoefficients=[1,2,2]),
    OrderParameter(type=0,
                   id=2,
                   speciesIDs=[3,4,5],
                   speciesCoefficients=[1,2,2])]

    theta = 1
    productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=5, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=11, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
    degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta),
                            ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
    reactionRateConstants = productionConstants + degradationConstants

    simParams = [SimulationParameter(key='maxSteps',val=str(int(1e10))),
                 SimulationParameter(key='maxTime',val='1e10'),
                 SimulationParameter(key='maxWorkUnitSteps',val=str(int(1e15))),
                 SimulationParameter(key='writeIntervalOP',val='%.10f' % (1.0/(.25*theta)))]

    for key,val in phaseCheckDict.items():
        if val is not None:
            simParams.append(SimulationParameter(key=key, val=str(int(float(val)))))

    tilings = [
    Tiling(id=0,
           orderParameterID=0,
           type=0,
           edges=np.linspace(-27,27,13)),
    Tiling(id=19,
           orderParameterID=0,
           type=0,
           edges=np.linspace(-25,25,13)),
    Tiling(id=1,
           orderParameterID=1,
           type=0,
           edges=np.arange(100)),
    Tiling(id=2,
           orderParameterID=2,
           type=0,
           edges=np.arange(100)),
    Tiling(id=3,
           orderParameterID=0,
           type=0,
           edges=np.arange(-100,100)),
    Tiling(id=199,
           orderParameterID=0,
           type=0,
           edges=np.linspace(-30,30,16)),
    Tiling(id=7,
           orderParameterID=0,
           type=0,
           edges=np.linspace(-25,25,11)),
    Tiling(id=27194,
           orderParameterID=0,
           type=0,
           edges=np.linspace(-20,20,5))]

    ffluxInput.AddTilings(tilings=tilings, currentTilingID=0)
    ffluxInput.SetInitialSpeciesCounts(iSCs=iSCs)
    ffluxInput.SetInitialSpeciesCountsBackward(iSCBs=iSCBs)
    ffluxInput.SetOrderParameters(ops=ops)
    ffluxInput.SetReactionRateConstants(rRates=reactionRateConstants)
    ffluxInput.SetSimulationParameters(simParams=simParams)
    ffluxInput.Close()

    Run(execPath, lmArgs)

def Main():
    parser = ArgumentParser('script to test out a complete Forward Flux Lattice Microbes run')

    parser.add_argument('execPath',                                  help='path to lmes (the Lattice Microbes executable)')

    parser.add_argument('-c', '--cpu', dest='c', default='5')
    parser.add_argument('-cr', '--cpus-per-runner', dest='cr', default='1')
    parser.add_argument('-gr', '--gpus-per-runner', dest='gr', default='1/4')
    parser.add_argument('-ff', '--output-format', dest='ff', default='hdf5')
    parser.add_argument('-fo', '--output-file', dest='fo')

    parser.add_argument('-mcz', '--maxCrossingsZero',                help='max crossing to record for phase zero')
    parser.add_argument('-mtz', '--maxTimeZero',                     help='max time to run phase zero for')
    parser.add_argument('-mcn', '--maxCrossingsN',                   help='max crossing to record for phase N')
    parser.add_argument('-mtn', '--maxTimeN',                        help='max time to run phase N for')

    parser.add_argument('-f', '--fullLength', action='store_true',   help='set this flag to do a test run using the default "best" parameters for forward flux')
    parser.add_argument('--sfile', action='store_true',              help='set this flag to use SFile output. Equivalent to -ff sfile -fo biphasic_switch.sfile')
    parser.add_argument('-t', '--timingLength', action='store_true', help='set this flag to do a test run that should last for at least a minute in both phase zero and the combined total of the rest of the phases')

    kwargs = vars(parser.parse_args())
    print_(kwargs)

    phaseCheckDictOverride = {key:val for key,val in ((key, kwargs.pop(key)) for key in ('maxCrossingsZero', 'maxTimeZero', 'maxCrossingsN', 'maxTimeN')) if val is not None}
    kwargs['phaseCheckDictOverride'] = phaseCheckDictOverride
    
    if kwargs.pop('sfile'):
        kwargs['ff'] = 'sfile'
        kwargs['fo'] = 'biphasic_switch.sfile'
        try:
            os.remove('biphasic_switch.sfile')
        except OSError:
            pass

    lmArgs = [tok for tup in ((key,val) for key,val in (('-%s' % key, kwargs.pop(key)) for key in ('c', 'cr', 'gr', 'ff', 'fo')) if val is not None) for tok in tup]
    kwargs['lmArgs'] = lmArgs
    
    _Main(**kwargs)

def Run(execPath, lmArgs):
    cmdToks = [execPath] + ['-sl', 'lm::cme::GillespieDSolver', '-f', 'biphasic_switch.lm', '-fflux', '-intout'] + lmArgs
    print_('running with:')
    print_(' '.join(cmdToks))
    p = subprocess.Popen(cmdToks)
    p.wait()

    # after this script sets up biphasic_switch.lm, the simulation can be rerun directly with:
    # ../build/lmes -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm" -intout

if __name__=='__main__':
    Main()
