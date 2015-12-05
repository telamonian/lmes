import getpass
import numbers
import os, sys
import re
import saga
from shutil import copy2 as cp
import tempfile
import uuid

import lmFile
from helper import *


class Job(object):
    '''
    copy_to: a list of tuples of the form ('file path on local to copy to remote', 'directory path of destination directory on remote')
    copy_from: a list of tuples of the form ('file path on remote to copy to local', 'directory path of destination directory on local')
    '''
    jobTypeName = 'job'
    
    def __init__(self, **kwargs):
        for key in kwargs:
            if key in self.__class__.keywords:
                self.__setattr__(key, kwargs[key])
            elif kwargs[key] is None:
                pass
            else:
                print key
                print self.__class__.keywords
                raise
        for key in self.__class__.keywords:
            if key not in kwargs:
                self.__setattr__(key, None)
        self.Init()
        self.CreateJobDescription()
      
    def _Init(self, callingClassName):
        if callingClassName=='Job':
    #         if self.error==None:
    #             self.error = 'lm.err'
            if self.host==None:
                self.host = 'localhost'
    #         if self.output==None:
    #             self.output = 'lm.log'
            if self.cpu_count!=None:
                self.total_cpu_count = self.cpu_count
            else:
                self.total_cpu_count = 1
            if self.user_id==None:
                self.user_id = getpass.getuser()
    
    def Init(self):
        for cls in self.__class__.mro():
            if '_Init' in cls.__dict__:
                cls._Init(self, cls.__name__)
    
    def _CopyTo(self, localPath, remotePath):
        '''
        copy from localPath to remotePath
        '''
        localFileUrl = 'file://%s' % PathJoin('localhost', localPath)
        remoteFileUrl = 'sftp://%s' % PathJoin(self.host, remotePath)
        remoteDirUrl = os.path.split(remoteFileUrl)[0]

        print('copying local file to remote: %s ---> %s') % (localFileUrl, remoteFileUrl)

        remoteSagaDir = saga.filesystem.Directory(remoteDirUrl, saga.filesystem.CREATE_PARENTS, session=self.runner.session)
        localSagaFile = saga.filesystem.File(localFileUrl, saga.filesystem.CREATE, session=self.runner.session)

        localSagaFile.copy(remoteFileUrl)
        localSagaFile.close()
        remoteSagaDir.close()

    def CopyTo(self, srcDstTups=None):
        '''
        for every tuple in job.copy_to (or srcDstTups if that arg is specified), copies local file at path in tuple[0] to remote directory path in tuple[1]
        '''
        if srcDstTups is None:
            srcDstTups = self.copy_to
        for localPath,remotePath in srcDstTups:
            fileName = os.path.split(localPath)[-1]
            if not os.path.isabs(localPath):
                remotePath = PathJoin(os.getcwd(), fileName)
            if remotePath=='':
                remotePath = PathJoin(self.working_directory, fileName)
            elif not os.path.isabs(remotePath):
                remotePath = PathJoin(self.working_directory, remotePath)
            self._CopyTo(localPath, remotePath)

    def _CopyFrom(self, job, relativeToWorkingDirectory=False):
        '''
        for every tuple in job.copy_from, copies remote file at path in tuple[0] to local directory path in tuple[1]
        '''
        for remoteSrc,localDir in job.copy_to:
            localDirUrl = 'file://%s' % os.path.join('localhost', localDir)
            remoteSrcUrl = 'sftp://%s' % os.path.join(job.host, remoteSrc)
            print('copying remote file to local: %s ---> %s') % (localDirUrl, remoteSrcUrl)

            localDirSaga = saga.filesystem.Directory(localDirUrl, saga.filesystem.CREATE, session=self.runner.session)
            remoteSrcSaga = saga.filesystem.File(remoteSrcUrl, session=self.runner.session)

            remoteSrcSaga.copy(localDirSaga.get_url())
    
    def CreateJobDescription(self):
        self.jd = saga.job.Description()
        for key in self.__class__.keywords_description:
            if hasattr(self, key) and self.__getattribute__(key)!=None:
                self.jd.__setattr__(key, self.__getattribute__(key))
#         self.jd_sge.environment       = {'SCRATCHFILE': 'testfile'}
#         self.jd_sge.wall_time_limit   = 1 # minutes
#         self.jds_sge[i].executable        = self.arguments_sge[i]  #r'mpirun'
#         self.jds_sge[i].arguments         = [r'-n', r'$NUMNODES', r'-f', r'$TMPDIR/mpich.hosts', self.executable_sge, r'--resource-map=\$TMPDIR/machine-resources', self.lm_args, r'-f', r'$SCRATCHFILE']
# 
#         self.jds_sge[i].working_directory = self.working_directory
#         self.jds_sge[i].output            = 'test_project.out'
#         self.jds_sge[i].error             = 'test_project.err'
#         
#         self.jds_sge[i].total_cpu_count   = self.total_cpu_count
#         self.jds_sge[i].spmd_variation    = self.pe # translates to the qsub -pe flag
# #         self.jd_sge.total_physical_memory = 1024 # Memory requirements in Megabyte
# 
#         self.jds_sge[i].queue             = self.queue
#         self.jds_sge[i].project           = "test_project"

    def SetRunner(self, runner):
        self.runner = runner
        runner.AddJob(self)

    def WriteTo(self, txtDstTups=None):
        '''
        for every tuple (txt, dst) in self.write_to (or txtDstTups), write txt to a temp file and then pass the schlimiel to .CopyTo()
        '''
        if txtDstTups is None:
            txtDstTups = self.write_to
        srcDstTups = []

        for txt,dst in txtDstTups:
            tmpFileDescriptor,tmpPath = tempfile.mkstemp()
            with os.fdopen(tmpFileDescriptor, 'w+') as tmp:
                tmp.write(txt)
            srcDstTups.append((tmpPath, dst))

        self.CopyTo(srcDstTups=srcDstTups)

        for tmpPath,dst in srcDstTups:
            os.unlink(tmpPath)

    @classmethod
    def _InitKeywords(cls, mro):
        return ('arguments','copy_to','copy_from', 'cpu_count','environment', 'error','executable','host', 'name', 'output','pass_exe','user_id','working_directory')
    
    @classmethod
    def InitKeywords(cls):
        cls.keywords = cls._InitKeywords(cls.mro())
    
    @classmethod
    def InitKeywordsDescription(cls):
        '''
        subset of keywords used for creating the composed saga.job.Description object
        '''
        cls.keywords_description = ('arguments','environment','error','executable','name','output','working_directory')

Job.InitKeywords()
Job.InitKeywordsDescription()

class JobLM(Job):
    jobTypeName = 'lm'
    
    def _Init(self, callingClassName):
        if callingClassName=='JobLM':
            self.SetLMArgs()
            self.SetReplicateRange()
    
    def ApplyInputTup(self):
        '''
        too complex
        use name of type of each input tuple to construct the name of the function in lmFile.Input used to set said tuple type
        '''
        for inputTup in self.lm_input_tups:
            tupTypeName = type(inputTup).__name__
            self.lmF.__getattribute__('Set%s' % tupTypeName)(inputTup)
        self.lmF.Flush()
        
    def CopyToLm(self):
        self.MkTmpLm()
        if self.lm_input_tups!=None:
            self.ApplyInputTup()
        if self.lm_sampling_rate is not None:
            if isinstance(self.lm_sampling_rate, dict):
                self.SetSamplingRate(**self.lm_sampling_rate)
            else:
                self.SetSamplingRate(rate=self.lm_sampling_rate)
        if self.lm_sampling_time:
            self.SetSamplingTime(time=self.lm_sampling_time)
        self.lmRemotePath = PathJoin(self.working_directory, self.lmName)
        self._CopyTo(self.lmTmpPath, self.lmRemotePath)
        self.RmTmpLm()
    
    def MkTmpLm(self):
        '''
        make temporary copy of lm_file in system tmp directory
        '''
        self.lmPath = self.lm_file_path
        self.lmName = os.path.split(self.lm_file_path)[-1]
        self.lmTmpPath = PathJoin(tempfile.gettempdir(), str(uuid.uuid4())+self.lmName)
        cp(self.lmPath, self.lmTmpPath)
        self.lmF = lmFile.Input(self.lmTmpPath)
    
    def RmTmpLm(self):
        '''
        remove temporary copy of lm_file from system tmp directory
        '''
        self.lmF.Close()
        os.remove(self.lmTmpPath)
    
    def SetLMArgs(self):
        if self.lm_args!=None:
            # check to make sure that the -a argument hasn't already been set
            founda = False
            for i,argument in enumerate(self.arguments):
                try:
                    if argument.strip()=='-a':
                        founda = i
                except AttributeError:
                    pass
            if founda:
                raise
            
            self.arguments.append('-a')
            self.arguments.append('"%s"' % self.lm_args)
    
    def SetReplicateRange(self):
        if self.lm_replicate_range!=None:
            lenrr = len(self.lm_replicate_range)
            if lenrr==2:
                replicateString = '%d-%d' % tuple(self.lm_replicate_range)
            else:
                replicateString = ('%d,'*(lenrr - 1) + '%d') % tuple(self.lm_replicate_range)
            # replace the -r argument and the token immediately following it. the -r argument should be in a long string following the -a argument
            founda = False
            for i,argument in enumerate(self.arguments):
                try:
                    if argument.strip()=='-a':
                        founda = i
                except AttributeError:
                    pass
            if founda:
                if '-r ' in self.arguments[founda+1]:
                    self.arguments[founda+1] = re.sub('(-r)\s+(\d+[,-]?)+', '\1 %d-%d' % tuple(self.lm_replicate_range), self.arguments[founda+1])
                else:
                    self.arguments[founda+1] = ('"-r %s ' % replicateString) + self.arguments[founda+1].lstrip('"')
            else:
                self.arguments.append('-a')
                self.arguments.append('"-r %s"' % replicateString)
    
    def SetSamplingRate(self, rate='auto', weight=1):
        '''
        if auto:
            sets sampling rate on the basis of the slowest simple reaction rate. rate is then multiplied by weight
        elif rate is a number:
            sets sampling rate to rate
        else:
            raise a ValueError
        '''
        if rate=='auto':
            reactionRateConstants = self.lmF.GetReactionRateConstants()
            rateToSet = float(weight)/np.min(reactionRateConstants[:,0])
        elif isinstance(rate, numbers.Number):
            # the arg is a python numeric type
            rateToSet = rate
        elif isinstance(rate, str):
            # the arg is a string, try to convert it to a number
            try:
                rateToSet = int(rate)
            except ValueError:
                rateToSet = float(rate)
        else:   
            raise ValueError
        rateToSet = '%.5f' % rateToSet
        simParam = lmFile.SimulationParameter(key='writeInterval', val=rateToSet)
        self.lmF.SetSimulationParameter(simParam=simParam)
        self.lmF.Flush()
    
    def SetSamplingTime(self, time='auto', leastLikelyRate=1e8):
        '''
        if auto:
            sets total sampling time based on a combination of sampling rate and known switching time (really the rate of the least likely event) for the system at hand
        elif time is a number:
            sets the total sampling time to time
        else:
            raise a ValueError
        '''
        if time=='auto':
            reactionRateConstants = self.lmF.GetReactionRateConstants()
            totalRunTimeToSet = float(1)/np.min(reactionRateConstants[:,0]) * leastLikelyRate
        elif isinstance(time, numbers.Number):
            # the arg is a python numeric type
            totalRunTimeToSet = time
        elif isinstance(time, str):
            # the arg is a string, try to convert it to a number
            try:
                totalRunTimeToSet = int(time)
            except ValueError:
                totalRunTimeToSet = float(time)
        else:   
            raise ValueError
        totalRunTimeToSet = '%.5f' % totalRunTimeToSet
        simParam = lmFile.SimulationParameter(key='maxTime', val=totalRunTimeToSet)
        self.lmF.SetSimulationParameter(simParam=simParam)
        self.lmF.Flush()
    
    @classmethod
    def _InitKeywords(cls, mro):
        # avoid repepitive addition
        if cls.__name__=='JobLM':
            additionalKeywords = ('lm_args','lm_sampling_rate','lm_sampling_time','lm_file_path','lm_input_tups','lm_replicate_range')
        else:
            additionalKeywords = ()
        return mro[mro.index(cls) + 1]._InitKeywords(mro) + additionalKeywords
    
JobLM.InitKeywords()

class JobSGE(Job):
    jobTypeName = 'sge'
    
    def _Init(self, callingClassName):
        if callingClassName=='JobSGE':
            if 'xanthus' in self.host:
                self.environment = {'SAGA_HOSTNAME': 'xanthus'}
                if self.queue=='gpu':
                    self.queue = 'gpu-1'
                    self.pe = self.spmd_variation = 'mpi-cuda'
                else:
                    self.queue = 'smp-1'
                    self.pe = self.spmd_variation = 'mpi'
            elif 'kirin' in self.host:
                self.environment = {'SAGA_HOSTNAME': 'kirin'}
                self.queue = 'normal'
                self.pe = self.spmd_variation = 'kirin-pe'
            else:
                self.spmd_variation = self.pe
            if self.total_cpu_count==None:
                self.total_cpu_count = 1
    
    @classmethod
    def _InitKeywords(cls, mro):
        # avoid repepitive addition
        if cls.__name__=='JobSGE':
            additionalKeywords = ('pe', 'project', 'queue', 'total_cpu_count')
        else:
            additionalKeywords = ()
        return mro[mro.index(cls) + 1]._InitKeywords(mro) + additionalKeywords
    
    @classmethod
    def InitKeywordsDescription(cls):
        cls.keywords_description = super(cls,cls).keywords_description + ('project', 'queue', 'total_cpu_count','spmd_variation')

JobSGE.InitKeywords()
JobSGE.InitKeywordsDescription()

class JobShell(Job):
    jobTypeName = 'shell'

class JobSlurm(Job):
    jobTypeName = 'slurm'
    
    def _Init(self, callingClassName):
        if callingClassName=='JobSlurm':
            if 'marcc' in self.host:
                self.environment = {'SAGA_HOSTNAME': 'marcc'}
                if self.queue==None:
                    self.queue = 'parallel'
            
            self.exclusive = True
            self.export = 'ALL'
            self.mail_type = 'end'
            self.job_contact = self.user_mail   
            self.wall_time_limit = 10080
            
            cpus_per_node = 24
            if not self.cpu_count % cpus_per_node==0:
                raise
            self.number_of_processes = self.cpu_count/cpus_per_node
            self.processes_per_host = 1
    
    @classmethod
    def _InitKeywords(cls, mro):
        # avoid repepitive addition
        if cls.__name__=='JobSlurm':
            additionalKeywords = ('project', 'queue', 'total_cpu_count', 'user_mail')
        else:
            additionalKeywords = ()
        return mro[mro.index(cls) + 1]._InitKeywords(mro) + additionalKeywords
    
    @classmethod
    def InitKeywordsDescription(cls):
        cls.keywords_description = super(cls,cls).keywords_description + ('exclusive', 'export', 'job_contact', 'mail_type', 'number_of_processes', 'processes_per_host', 'project', 'queue', 'total_cpu_count', 'wall_time_limit')

JobSlurm.InitKeywords()
JobSlurm.InitKeywordsDescription()

# combined Job types
class JobSGELM(JobLM, JobSGE):
    jobTypeName = 'sgelm'
    
    @classmethod
    def GetMRO(cls):
        print(cls.mro())
JobSGELM.InitKeywords()

class JobShellLM(JobLM, JobShell):
    jobTypeName = 'shelllm'
JobShellLM.InitKeywords()

class JobSlurmLM(JobLM, JobSlurm):
    jobTypeName = 'slurmlm'
JobSlurmLM.InitKeywords()