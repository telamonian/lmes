import operator
import os
import saga

class Runner(object):
    def __init__(self, jobs):
        self.host = jobs[0].host
        self.jobs = jobs
        self.jobs_saga = []
        self.user_id = jobs[0].user_id
        
        self.CreateContext()
        self.CreateSession()
        job_types = map(operator.methodcaller('__getattribute__','type'), jobs)
        if 'sge' in job_types:
            self.CreateJobServiceSGE()
        if 'shell' in job_types:
            self.CreateJobServiceShell()
        for job in jobs:
            if job.copy_to!=None:
                self.CopyTo(job)
            self.CreateJob(job)
    
        #self.CopyFrom(job)

    def CopyTo(self, job):
        '''
        for every tuple in job.copy_to, copies local file at path in tuple[0] to remote directory path in tuple[1]
        '''
        for localSrc,remoteDir in job.copy_to:
            remoteDirUrl = 'sftp://%s' % os.path.join(self.host, remoteDir)
            remoteDirSaga = saga.filesystem.Directory(remoteDirUrl, saga.filesystem.CREATE, session=self.session)
            localSrcUrl = 'file://%s' % os.path.join('localhost', localSrc)
            localSrcSaga = saga.filesystem.File(localSrcUrl)
            localSrcSaga.copy(remoteDirSaga.get_url())

    def CopyFrom(self, job):
        '''
        for every tuple in job.copy_from, copies remote file at path in tuple[0] to local directory path in tuple[1]
        '''
        for remoteSrc,localDir in job.copy_to:
            localDirUrl = 'file://%s' % os.path.join('localhost', localDir)
            localDirSaga = saga.filesystem.Directory(localDirUrl, saga.filesystem.CREATE, session=self.session)
            remoteSrcUrl = 'sftp://%s' % os.path.join(self.host, remoteSrc)
            remoteSrcSaga = saga.filesystem.File(remoteSrcUrl)
            remoteSrcSaga.copy(localDirSaga.get_url())

    def CreateContext(self):
        # Your ssh identity on the remote machine.
        self.ctx = saga.Context("ssh")
        
        # Change e.g., if you have a differnent username on the remote machine
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
        # Create a job service object that represent a remote sge cluster.
        # The keyword 'sge' in the url scheme triggers the SGE adaptors
        # and '+ssh' enables SGE remote access via SSH.
        self.js_shell = saga.job.Service(self.url_shell,session=self.session)
    
    def CreateJob(self, job):
        if job.type=='sge':
            self.CreateJobSGE(job.jd)
        elif job.type=='shell':
            self.CreateJobShell(job.jd)
    
    def CreateJobSGE(self, job_description):
        # Create a new job from the job description. The initial state of 
        # the job is 'New'.
        self.jobs_saga.append(self.js_sge.create_job(job_description))
        
    def CreateJobShell(self, job_description):
        # Create a new job from the job description. The initial state of 
        # the job is 'New'.
        self.jobs_saga.append(self.js_shell.create_job(job_description))
    
    def Finish(self):
        self.js_sge.close()
        self.js_shell.close()
    
    def Run(self, job, job_saga):
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
 
        if job.type=='shell':
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
 
        
        return 0
    
    def RunAll(self):
        for job,job_saga in zip(self.jobs, self.jobs_saga):
            self.Run(job, job_saga)
        self.Finish()
    
    #def RunBase