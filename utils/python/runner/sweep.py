from job import JobSGELM, JobShellLM
# this hackishness imports all of the things in the inputTypes list in lmFile
import lmFile
from lmFile import GetTypes,inputTypes; GetTypes(inputTypes,__name__)
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
    def __init__(self, cpu_count, host, lm_bin, lm_file_path, rootPath, sweepTupX=None, sweepTupY=None, type='shell', user_id=None):
        self.cpu_count = cpu_count
        self.host = host
        self.jobs = []
        self.lm_bin = lm_bin
        self.lm_file_path = lm_file_path
        self.lmFileName = os.path.split(lm_file_path)[-1]
        self.rootPath = rootPath
        
        if sweepTupX==None:
            self.sweepTupX = [None]
        else:
            self.sweepTupX = sweepTupX
        if sweepTupY==None:
            self.sweepTupY = [None]
        else:
            self.sweepTupY = sweepTupY
        
        if type=='sge':
            self.jobType = JobSGELM
        elif type=='shell':
            self.jobType = JobShellLM
    
    def Run(self):
        self.runner.Run()
    
    def Setup(self):
        self.runner = Runner()
        for labelX, inputTupsX in self.sweepTupX:
            for labelY, inputTupsY in self.sweepTupY:
                jobDict = {'arguments': ['-a', '-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5', '-s', self.lm_file_path, '-x', self.lm_bin],
                           'copy_to': [[os.path.join(thisScriptsPath, 'sge_glue.sh'), '']],
                           'copy_from': [],
                           'cpu_count': self.cpu_count,
                           'error': 'lm.err',
                           'executable': 'sge_glue.sh',
                           'host': self.host,
                           'lm_file_path': self.lm_file_path,
                           'lm_input_tups': inputTupsX+inputTupsY,
                           'output': 'lm.log',
                           'user_id': None,
                           'working_directory': os.path.join(self.rootPath, '%s_%s' % (labelX, labelY))}
                currentJob = self.jobType(**jobDict)
                currentJob.SetRunner(self.runner)
                self.jobs.append(currentJob)
        self.runner.Setup()
                