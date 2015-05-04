from helper import PathJoin
from job import JobSGELM, JobShellLM
# this hackishness imports all of the things in the inputTypes list in lmFile
#import lmFile
#from lmFile import GetTypes,inputTypes; GetTypes(inputTypes,__name__)
from lmFile import *
import os
from runner import Runner

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

class SweepTup(object):
    def __init__(self, inputTupss, label, labelVals):
        '''
        inputTupps: list of lists (LoL) of InputTup data instances
        label: what to call the parameter being altered (by this sweep) in the directory names in the output
        labelVals: a list of values, one for each list in the inputTupps LoL, that is used in conjunction with label to name the directories containing the output of the sweep
        '''
        self.inputTupss = inputTupss
        self.label = label
        self.labelVals = labelVals
    
    def __getitem__(self, i):
        return (self.label % self.labelVals[i]), self.inputTupss[i]
    
    def __iter__(self):
        for i in range(len(self.inputTupss)):
            yield self[i]
    
class Sweep(object):
    def __init__(self, cpu_count, host, lm_bin, lm_file_path, rootPath, autosetSamplingRate=False, autosetSamplingTime=False, diagonal=False, inputTupsDefault=None, lmArgsGpusPerReplicate=0, replicateRange=(1,10), sweepTupX=None, sweepTupY=None, type='shell', useForwardFlux=False, user_id=None):
        self.autosetSamplingRate = autosetSamplingRate
        self.autosetSamplingTime = autosetSamplingTime
        self.cpu_count = cpu_count
        self.diagonal = diagonal
        self.host = host
        self.jobs = []
        self.lm_bin = lm_bin
        self.lm_file_path = lm_file_path
        self.lmFileName = os.path.split(lm_file_path)[-1]
        self.replicateRange = replicateRange
        self.rootPath = rootPath
        self.user_id = user_id
        
        self.lm_args_dict = {'cpus_per_replicate':           ('-cr',     '1'),
                             'file_format':                  ('-ff',     'hdf5'),
                             'gpus_per_replicate':           ('-gr',     lmArgsGpusPerReplicate),
                             'solver_class':                 ('-sl',     'lm::cme::GillespieDSolver')}
        if useForwardFlux:
            self.lm_args_dict['use_forward_flux_sampling'] = ('-fflux',  '')
        
        self.inputTupsDefault = inputTupsDefault
        self.sweepTupX = sweepTupX
        self.sweepTupY = sweepTupY
        
        if type=='sge':
            self.jobType = JobSGELM
        elif type=='shell':
            self.jobType = JobShellLM
    
    def GetLMArgs(self):
        self.lm_args = ''
        for (opt, val) in self.lm_args_dict.itervalues():
            self.lm_args = self.lm_args + '{opt} {val} '.format(opt=opt, val=val)
    
    def Run(self):
        self.runner.Run()
    
    def Setup(self):
        self.GetLMArgs()
        self.runner = Runner()
        for i,(labelX,inputTupsX) in enumerate(self.sweepTupX):
            for j,(labelY,inputTupsY) in enumerate(self.sweepTupY):
                if self.diagonal and not i==j:
                    continue
                working_directory = PathJoin(self.rootPath, '%s_%s' % (labelX, labelY))
                jobDict = {'arguments': ['-n', self.cpu_count, '-s', self.lm_file_path, '-x', self.lm_bin],
                           'copy_to': [[PathJoin(thisScriptsPath, 'sge_glue.sh'), '']],
                           'copy_from': [],
                           'cpu_count': self.cpu_count,
                           'error': 'lm.err',
                           'executable': PathJoin(working_directory, 'sge_glue.sh'),
                           'host': self.host,
                           'lm_autoset_sampling_rate': self.autosetSamplingRate,
                           'lm_autoset_sampling_time': self.autosetSamplingTime,
                           'lm_args': self.lm_args,
                           'lm_file_path': self.lm_file_path,
                           'lm_input_tups': inputTupsX+inputTupsY+self.inputTupsDefault,
                           'lm_replicate_range': self.replicateRange,
                           'output': 'lm.log',
                           'user_id': self.user_id,
                           'working_directory': working_directory}
                currentJob = self.jobType(**jobDict)
                currentJob.SetRunner(self.runner)
                self.jobs.append(currentJob)
        self.runner.Setup()
