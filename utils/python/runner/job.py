import getpass
import saga

class Job(object):
    '''
    copy_to: a list of tuples of the form ('file path on local to copy to remote', 'directory path of destination directory on remote')
    copy_from: a list of tuples of the form ('file path on remote to copy to local', 'directory path of destination directory on local')
    '''
    keywords = ('arguments','copy_to','copy_from', 'environment', 'error','executable','host','output','user_id','working_directory')
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
        if self.user_id==None:
            self.user_id = getpass.getuser()
            
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

class JobSGE(Job):
    type = 'sge'
    
    def Init(self):
        super(type(self), self).Init()
        if self.host=='xanthus':
            if self.queue=='gpu':
                self.queue = 'gpu-1'
                self.pe = self.spmd_variation = 'mpi-cuda'
            else:
                self.queue = 'smp-1'
                self.pe = self.spmd_variation = 'mpi'
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