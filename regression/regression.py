from argparse import ArgumentParser, SUPPRESS
import os,sys
from pathlib import Path
import shutil
from six import print_
import subprocess

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
thisScriptDirPath = Path(thisScriptDir)

class Regression(object):
    defaultExecPath = thisScriptDirPath / '../build/lmes'
    defaultStartingInputPath = thisScriptDirPath / 'wo_fflux.biphasic_switch.lm'
    defaultFinalInputPath = 'biphasic_switch.lm'

    defaultLMArgs = ['-f', 'biphasic_switch.lm']
    helpMessage = 'base class for doing regression testing on Lattice Microbes'

    def BuildInput(self, startingInputPath, f, **kwargs):
        try:
            os.remove(str(f))
        except OSError:
            pass
        shutil.copy(str(startingInputPath), str(f))
        # lm_sbml_import $${filename} bimolecular_with_limits.sbml

        self._BuildInput(**kwargs)

    def BuildLMArgs(self, kwargs):
        lmFlags = ('fflux', 'intout')
        lmOptions = ('c', 'cr', 'gr', 'f', 'ff', 'fo', 'fp', 'sl')

        kwargs['lmArgs'] = ['-%s' % flag for flag in lmFlags if kwargs[flag]]
        kwargs['lmArgs']+=[tok for tup in ((option,val) for option,val in (('-%s' % option, kwargs[option]) for option in lmOptions) if val is not None) for tok in tup]

    def CleanSFileOutput(self, **kwargs):
        if kwargs['ff']=='sfile' and kwargs['fo']=='biphasic_switch.sfile':
            try:
                os.remove('biphasic_switch.sfile')
            except OSError:
                pass

    def Exec(self, execPath, lmArgs):
        cmdToks = [str(execPath)] + self.defaultLMArgs + lmArgs
        print_('running with:')
        print_(' '.join(cmdToks))
        # p = subprocess.Popen(cmdToks)
        # p.wait()

        with subprocess.Popen(cmdToks, stdout=subprocess.PIPE, bufsize=1, universal_newlines=True) as p:
            for line in p.stdout:
                print_(line, end='')
            print_(p.communicate()[0], end='')

    def Main(self):
        kwargs = self.Parse()
        self.Run(**kwargs)

    def Parse(self):
        parser = ArgumentParser(self.helpMessage)

        parser.add_argument('execPath', default=self.defaultExecPath, nargs='?',         help='path to lmes (the Lattice Microbes executable). If left blank, defaults to ../build/lmes')
        parser.add_argument('--starting-input-path', dest='startingInputPath',
                            default=self.defaultStartingInputPath,                       help='path to the simulation input file that this script will build upon to get the input file for the test. Defaults to wo_fflux.biphasic_switch.lm')
        parser.add_argument('--build-input', action='store_true',                        help='if this flag is set, the Lattice Microbes input file for the regression test will be built but the test will not be run')

        parser.add_argument('-c', '--cpu', dest='c', default='5',                        help='total number of cpu cores that the simulation can use')
        parser.add_argument('-cr', '--cpus-per-runner', dest='cr', default='1',          help='number of cpu cores that the simulation will assign to each work unit runner. Can be fractional')
        parser.add_argument('-gr', '--gpus-per-runner', dest='gr', default='1/4',        help='number of gpus that the simulation will assign to each work unit runner. Can be fractional')
        parser.add_argument('-f', '--file', dest='f',
                            default=self.defaultFinalInputPath,                          help='simulation input file path')
        parser.add_argument('-ff', '--output-format', dest='ff', default='hdf5',         help='output file format')
        parser.add_argument('-fo', '--output-file', dest='fo',                           help='path to output file')
        parser.add_argument('-fp', '--output-prefix', dest='fp',                         help='The prefix to use for the record names')
        parser.add_argument('-sl', '--solver', dest='sl',
                            default='lm::avx::GillespieDSolverAVX',                      help='fully qualified c++ class name of solver to use during simulation. should be one of (lm::cme::GillespieDSolver | lm::avx::GillespieDSolverAVX)')
        parser.add_argument('-intout', '--intermediate-output',
                            action='store_true', dest='intout',                          help='output some extra data during certain kinds of simulations')

        # general simulation parameters
        parser.add_argument('-t', '--theta', default=SUPPRESS, type=float,               help='scaling factor for the rates of protein production and degradation in the test Genetic Toggle Switch system.')
        parser.add_argument('--maxWorkUnitSteps', default=SUPPRESS,                      help='max number of steps in a single work unit')
        parser.add_argument('--writeInterval', default=SUPPRESS,                         help='the period at which every trajectory will write out the state of its species counts')
        parser.add_argument('--orderParameterWriteInterval', default=SUPPRESS,           help='the period at which every trajectory will write out the state of its order parameter values')
        parser.add_argument('--extra-input', action='store_true',                        help="add some extra order parameters and tilings to the .lm input file. Meant for use in analysis only (ie, don't use in conjunction with execPath)")
        parser.add_argument('--quick-test', action='store_true',                         help='use presets for simulation parameters, etc that will result in roughly the quickest possible simulation that will still give useful results for testing purposes')
        parser.add_argument('--sfile', action='store_true',                              help='set this flag to use SFile output. Equivalent to -ff sfile -fo biphasic_switch.sfile')

        # forward flux specific simulation parameters
        parser.add_argument('--fflux', action='store_true',                              help='set this flag to do a Forward Flux simulation instead of the deafult Replicate simulation')
        parser.add_argument('-psc', '--pilotStageCount', default=SUPPRESS,               help='fixed number of trajectories to launch during each phase of the pilot stage for FFPilot')
        parser.add_argument('-pg', '--precisionGoal', default=SUPPRESS,                  help='precision goal for FFPilot')
        parser.add_argument('-pgc', '--precisionGoalConfidence', default=SUPPRESS,       help='confidence level for precision goal for FFPilot')

        # replicate specific simulation parameters
        parser.add_argument('-fpt', '--firstPassageTimeSpecies', action='store_true',    help='set this flag to track species first passage times')
        parser.add_argument('-fptop', '--firstPassageTimeOrderParameters',
                                action='store_true',                                     help='set this flag to track order parameter first passage times')
        parser.add_argument('--maxSteps', default=SUPPRESS,                              help='max number of steps to run for a single replicate')
        parser.add_argument('--maxTime', default=SUPPRESS,                               help='max time to run for a single replicate')

        kwargs = vars(parser.parse_args())
        print_(kwargs)

        if kwargs['fflux']:
            kwargs['intout'] = True
        if kwargs.pop('sfile'):
            kwargs['ff'] = 'sfile'
            kwargs['fo'] = 'biphasic_switch.sfile'

        return kwargs

    def Run(self, **kwargs):
        if kwargs['build_input']:
            print_('Building lmes forward flux simulation input file without executing the test')
            self.BuildInput(**kwargs)
        else:
            print_('Building lmes forward flux simulation input file and then running a test')
            if kwargs['execPath']==self.defaultExecPath:
                print_('No execPath option set, using default path to Lattice Microbes executable: %s' % self.defaultExecPath)
            self.BuildInput(**kwargs)
            self.BuildLMArgs(kwargs)
            self.CleanSFileOutput(**kwargs)
            self.Exec(execPath=kwargs['execPath'], lmArgs=kwargs['lmArgs'])
