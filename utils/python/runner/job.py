import os, sys
# script_dir = os.path.dirname(os.path.abspath(__file__))
# script_dir_one_up = os.path.split(script_dir)[0]
# sys.path.append(script_dir_one_up)

import lmFile
import getpass
import saga
from shutil import copy2 as cp
import tempfile
import uuid

class Job(object):
    '''
    copy_to: a list of tuples of the form ('file path on local to copy to remote', 'directory path of destination directory on remote')
    copy_from: a list of tuples of the form ('file path on remote to copy to local', 'directory path of destination directory on local')
    '''
    keywords = ('arguments','copy_to','copy_from', 'cpu_count','environment', 'error','executable','host','lm_file_path','lm_input_tups','output','user_id','working_directory')
    # subset of keywords used for creating the composed saga.job.Description object
    keywords_description = ('arguments','environment','error','executable','output','working_directory')
    type = 'base'
    
    def __init__(self, **kwargs):
        for key in kwargs:
            if key in self.__class__.keywords:
                self.__setattr__(key, kwargs[key])
            else:
                raise
        for key in self.__class__.keywords:
            if key not in kwargs:
                self.__setattr__(key, None)
        self.Init()
        self.CreateJobDescription()
    
    def Init(self):
#         if self.error==None:
#             self.error = 'lm.err'
        if self.host==None:
            self.host = 'localhost'
#         if self.output==None:
#             self.output = 'lm.log'
        if self.lm_file_path!=None:
            self.MkTmpLm()
        if self.cpu_count!=None:
            self.total_cpu_count = self.cpu_count
        else:
            self.total_cpu_count = 1
        if self.user_id==None:
            self.user_id = getpass.getuser()
    
    def _CopyTo(self, localPath, remotePath):
        '''
        copy from localPath to remotePath
        '''
        localFileUrl = 'file://%s' % os.path.join('localhost', localPath)
        remoteFileUrl = 'sftp://%s' % os.path.join(self.host, remotePath)
        remoteDirUrl = os.path.split(remoteFileUrl)[0]
        remoteSagaDir = saga.filesystem.Directory(remoteDirUrl, saga.filesystem.CREATE_PARENTS, session=self.runner.session)
        print remoteFileUrl
        localSagaFile = saga.filesystem.File(localFileUrl, saga.filesystem.CREATE, session=self.runner.session)
        localSagaFile.copy(remoteFileUrl)
        localSagaFile.close()
        remoteSagaDir.close()
#         remoteDirUrl = 'sftp://%s' % os.path.join(self.host, remoteDir)
#         remoteSagaDir = saga.filesystem.Directory(remoteDirUrl, saga.filesystem.CREATE, session=self.session)
#         localFileUrl = 'file://%s' % os.path.join('localhost', localPath)
#         localSagaFile = saga.filesystem.File(localSrcUrl)
#         localSrcSaga.copy(remoteDirSaga.get_url())

    def CopyTo(self):
        '''
        for every tuple in job.copy_to, copies local file at path in tuple[0] to remote directory path in tuple[1]
        '''
        for localPath,remotePath in self.copy_to:
            fileName = os.path.split(localPath)[-1]
            if not os.path.isabs(localPath):
                remotePath = os.path.join(os.getcwd(), fileName)
            if remotePath=='':
                remotePath = os.path.join(self.working_directory, fileName)
            self._CopyTo(localPath, remotePath)

    def _CopyFrom(self, job, relativeToWorkingDirectory=False):
        '''
        for every tuple in job.copy_from, copies remote file at path in tuple[0] to local directory path in tuple[1]
        '''
        for remoteSrc,localDir in job.copy_to:
            localDirUrl = 'file://%s' % os.path.join('localhost', localDir)
            localDirSaga = saga.filesystem.Directory(localDirUrl, saga.filesystem.CREATE, session=self.session)
            remoteSrcUrl = 'sftp://%s' % os.path.join(job.host, remoteSrc)
            remoteSrcSaga = saga.filesystem.File(remoteSrcUrl)
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

class JobLMMixin(object):
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
        self.lmRemotePath = os.path.join(self.working_directory, self.lmName)
        self._CopyTo(self.lmTmpPath, self.lmRemotePath)
        self.RmTmpLm()
    
    def MkTmpLm(self):
        '''
        make temporary copy of lm_file in system tmp directory
        '''
        self.lmPath = self.lm_file_path
        self.lmName = os.path.split(self.lm_file_path)[-1]
        self.lmTmpPath = os.path.join(tempfile.gettempdir(), str(uuid.uuid4())+self.lmName)
        cp(self.lmPath, self.lmTmpPath)
        self.lmF = lmFile.Input(self.lmTmpPath)
    
    def RmTmpLm(self):
        '''
        remove temporary copy of lm_file from system tmp directory
        '''
        self.lmF.Close()
        os.remove(self.lmTmpPath)

class JobSGE(Job):
    type = 'sge'
    
    def Init(self):
        super(type(self), self).Init()
        if 'xanthus' in self.host:
            if self.queue=='gpu':
                self.queue = 'gpu-1'
                self.pe = self.spmd_variation = 'mpi-cuda'
            else:
                self.queue = 'smp-1'
                self.pe = self.spmd_variation = 'mpi'
        if 'kirin' in self.host:
            self.queue = 'normal'
            self.pe = self.spmd_variation = 'kirin-pe'
        else:
            self.spmd_variation = self.pe
        if self.total_cpu_count==None:
            self.total_cpu_count = 1
    
    @classmethod
    def InitKeywords(cls):
        cls.keywords = super(cls,cls).keywords + ('pe', 'project', 'queue', 'total_cpu_count')
    
    @classmethod
    def InitKeywordsDescription(cls):
        cls.keywords_description = super(cls,cls).keywords_description + ('project', 'queue', 'total_cpu_count','spmd_variation')

JobSGE.InitKeywords()
JobSGE.InitKeywordsDescription()

class JobShell(Job):
    type = 'shell'
    
JobSGELM = type('JobSGELM', (JobLMMixin,JobSGE), {})
JobShellLM = type('JobShellLM', (JobLMMixin,JobShell), {})