from helper import InList
import operator
import os
import re
import saga
import subprocess

class Runner(object):
    def __init__(self, jobs=None):
        if jobs==None:
            self.jobs = []
        else:
            self.jobs = jobs 
        self.jobs_saga = []

    def AddJob(self, job):
        self.jobs.append(job)
    
        #self.CopyFrom(job)

#     def CopyTo(self, job):
#         '''
#         for every tuple in job.copy_to, copies local file at path in tuple[0] to remote directory path in tuple[1]
#         '''
#         for localSrc,remoteDir in job.copy_to:
#             remoteDirUrl = 'sftp://%s' % os.path.join(job.host, remoteDir)
#             remoteDirSaga = saga.filesystem.Directory(remoteDirUrl, saga.filesystem.CREATE, session=self.session)
#             localSrcUrl = 'file://%s' % os.path.join('localhost', localSrc)
#             localSrcSaga = saga.filesystem.File(localSrcUrl)
#             localSrcSaga.copy(remoteDirSaga.get_url())
# 
#     def CopyFrom(self, job, relativeToWorkingDirectory=False):
#         '''
#         for every tuple in job.copy_from, copies remote file at path in tuple[0] to local directory path in tuple[1]
#         '''
#         for remoteSrc,localDir in job.copy_to:
#             localDirUrl = 'file://%s' % os.path.join('localhost', localDir)
#             localDirSaga = saga.filesystem.Directory(localDirUrl, saga.filesystem.CREATE, session=self.session)
#             remoteSrcUrl = 'sftp://%s' % os.path.join(job.host, remoteSrc)
#             remoteSrcSaga = saga.filesystem.File(remoteSrcUrl)
#             remoteSrcSaga.copy(localDirSaga.get_url())

    def CreateContext(self):
        if self.pass_exe is not None:
            self.ctx = saga.Context("UserPass")
            self.ctx.user_id = self.user_id
            # get the password by reading the stdout from an exe
            self.ctx.user_pass = subprocess.Popen(self.pass_exe, stdout=subprocess.PIPE).communicate()[0].strip()
        else:
            # Your ssh identity on the remote machine.
            self.ctx = saga.Context("ssh")
            
            # Change e.g., if you have a different username on the remote machine
            self.ctx.user_id = self.user_id
        
    
    def CreateSession(self):
        self.session = saga.Session()
        self.session.add_context(self.ctx)
    
    def CreateJobServiceBase(self):
        pass
    
    def CreateJobServiceSGE(self):
        self.url_sge = "sge+ssh://%s" % self.host
        # Create a job service object that represent a remote sge cluster.
        # The keyword 'sge' in the url scheme triggers the SGE adaptors
        # and '+ssh' enables SGE remote access via SSH.
        self.js_sge = saga.job.Service(self.url_sge,session=self.session)
    
    def CreateJobServiceShell(self):
        self.url_shell = "ssh://%s" % self.host
        self.js_shell = saga.job.Service(self.url_shell,session=self.session)
        
    def CreateJobServiceSlurm(self):
        self.url_slurm = "slurm+ssh://%s" % self.host
        self.js_slurm = saga.job.Service(self.url_slurm,session=self.session)
    
    def CreateJobSaga(self, job):
        if 'sge' in job.jobTypeName:
            self.CreateJobSagaSGE(job.jd)
        elif 'shell' in job.jobTypeName:
            self.CreateJobSagaShell(job.jd)
        elif 'slurm' in job.jobTypeName:
            self.CreateJobSagaSlurm(job.jd)
        else:
            raise
    
    def CreateJobSagaSGE(self, job_description):
        # Create a new job from the job description. The initial state of 
        # the job is 'New'.
        self.jobs_saga.append(self.js_sge.create_job(job_description))
        
    def CreateJobSagaShell(self, job_description):
        self.jobs_saga.append(self.js_shell.create_job(job_description))
    
    def CreateJobSagaSlurm(self, job_description):
        self.jobs_saga.append(self.js_slurm.create_job(job_description))
    
    def Finish(self):
        try:
            self.js_sge.close()
        except AttributeError:
            pass
        try:
            self.js_shell.close()
        except AttributeError:
            pass
        try:
            self.js_slurm.close()
        except AttributeError:
            pass
    
    def _RunJob(self, job, job_saga):
        # Check our job_saga's id and state
        print "job ID    : %s" % (job_saga.id)
        print "job State : %s" % (job_saga.state)
 
        # Now we can start our job_saga.
        print "\n...starting job_saga...\n"
        job_saga.run()
 
        print "job ID    : %s" % (job_saga.id)
        print "job State : %s" % (job_saga.state)
 
        # List all job_sagas that are known by the adaptor.
        # This should show our job_saga as well.
#         print "\nListing active job_sagas: "
#         for job_saga in self.js_shell.list():
#             print " * %s" % job_saga
 
        if 'shell' in job.jobTypeName:
            print "this job's working dir is %s" % job.jd.working_directory
            print "this job's exec is %s" % job.jd.executable
            print "this job's args are %s" % job.jd.arguments
            # wait for our job_saga to complete
            print "\n...waiting for job...\n"
            job_saga.wait()
 
            print "job State   : %s" % (job_saga.state)
            print "Exitcode    : %s" % (job_saga.exit_code)
            print "Exec. hosts : %s" % (job_saga.execution_hosts)
            print "Create time : %s" % (job_saga.created)
            print "Start time  : %s" % (job_saga.started)
            print "End time    : %s" % (job_saga.finished)
        else:
            print "this job's working dir is %s" % job.jd.working_directory
            print "this job's exec is %s" % job.jd.executable
            print "this job's args are %s" % job.jd.arguments
        return 0
    
    def Run(self):
        for job,job_saga in zip(self.jobs, self.jobs_saga):
            self._RunJob(job, job_saga)
        self.Finish()
        
    def Setup(self):
        self.host = self.jobs[0].host
        self.pass_exe = self.jobs[0].pass_exe
        self.user_id = self.jobs[0].user_id
        
        self.CreateContext()
        self.CreateSession()
        
        job_types = map(operator.methodcaller('__getattribute__','jobTypeName'), self.jobs)
        if InList('sge', job_types):
            self.CreateJobServiceSGE()
        if InList('shell', job_types):
            self.CreateJobServiceShell()
        if InList('slurm', job_types):
            self.CreateJobServiceSlurm()
            
        for job in self.jobs:
            if job.copy_to!=None:
                job.CopyTo()
            if hasattr(job, 'lm_file_path') and job.lm_file_path!=None:
                job.CopyToLm()
            self.CreateJobSaga(job)
    