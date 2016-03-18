from argparse import ArgumentParser
import os,sys
from six import print_
import subprocess

class Regression(object):
    defaultLMArgs = ['-sl', 'lm::avx::GillespieDSolverAVX', '-f', 'biphasic_switch.lm']
    helpMessage = 'base class for doing regression testing on Lattice Microbes'

    # def BuildInput(self, **kwargs):
    #     unimplemented in base class

    def BuildLMArgs(self, kwargs):
        lmFlags = ('fflux', 'intout')
        lmOptions = ('c', 'cr', 'gr', 'ff', 'fo')

        kwargs['lmArgs'] = ['-%s' % flag for flag in lmFlags if kwargs[flag]]
        kwargs['lmArgs']+= [tok for tup in ((option,val) for option,val in (('-%s' % option, kwargs[option]) for option in lmOptions) if val is not None) for tok in tup]

    def CleanSFileOutput(self, **kwargs):
        if kwargs['ff']=='sfile' and kwargs['fo']=='biphasic_switch.sfile':
            try:
                os.remove('biphasic_switch.sfile')
            except OSError:
                pass

    def Exec(self, execPath, lmArgs):
        cmdToks = [execPath] + self.defaultLMArgs + lmArgs
        print_('running with:')
        print_(' '.join(cmdToks))
        p = subprocess.Popen(cmdToks)
        p.wait()

    def Main(self):
        kwargs = self.Parse()
        self.Run(**kwargs)

    def Parse(self):
        parser = ArgumentParser(self.helpMessage)

        parser.add_argument('execPath', default='', nargs='?',                           help='path to lmes (the Lattice Microbes executable). If left blank, the regression test input file will be built but the test will not be run')

        parser.add_argument('-c', '--cpu', dest='c', default='5',                        help='total number of cpu cores that the simulation can use')
        parser.add_argument('-cr', '--cpus-per-runner', dest='cr', default='1',          help='number of cpu cores that the simulation will assign to each work unit runner. Can be fractional')
        parser.add_argument('-gr', '--gpus-per-runner', dest='gr', default='1/4',        help='number of gpus that the simulation will assign to each work unit runner. Can be fractional')
        parser.add_argument('-ff', '--output-format', dest='ff', default='hdf5',         help='output file format')
        parser.add_argument('-fo', '--output-file', dest='fo',                           help='path to output file')
        parser.add_argument('-intout', '--intermediate-output',
                            action='store_true', dest='intout',                          help='output some extra data during certain kinds of simulations')

        parser.add_argument('-t', '--theta', default=1,                                  help='scaling factor for the rates of protein production and degradation in the test Genetic Toggle Switch system.')

        parser.add_argument('-mcz', '--maxCrossingsZero',                                help='max crossing to record for phase zero')
        parser.add_argument('-mtz', '--maxTimeZero',                                     help='max time to run phase zero for')
        parser.add_argument('-mcn', '--maxCrossingsN',                                   help='max crossing to record for phase N')
        parser.add_argument('-mtn', '--maxTimeN',                                        help='max time to run phase N for')

        parser.add_argument('--fflux', action='store_true',                              help='set this flag to do a Forward Flux simulation instead of the deafult Replicate simulation')
        parser.add_argument('--sfile', action='store_true',                              help='set this flag to use SFile output. Equivalent to -ff sfile -fo biphasic_switch.sfile')

        kwargs = vars(parser.parse_args())
        print_(kwargs)

        if kwargs['fflux']:
            kwargs['intout'] = True
        if kwargs.pop('sfile'):
            kwargs['ff'] = 'sfile'
            kwargs['fo'] = 'biphasic_switch.sfile'

        return kwargs

    def Run(self, **kwargs):
        if not kwargs['execPath']:
            print_('Building lmes forward flux simulation input file without executing the test')
            self.BuildInput(**kwargs)
        else:
            print_('Building lmes forward flux simulation input file and then running a test')
            self.BuildInput(**kwargs)
            self.BuildLMArgs(kwargs)
            self.CleanSFileOutput(**kwargs)
            self.Exec(execPath=kwargs['execPath'], lmArgs=kwargs['lmArgs'])
