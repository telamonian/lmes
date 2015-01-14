from job import JobSGE, JobShell
from runner import Runner

    
if __name__=='__main__':
    runnerShellDict = {'arguments_shell':['>', 'listing.out'], 'executable':'ls', 'user_id':'cklein13', 'working_directory':'$HOME'}
    runnerShell = Runner(**runnerShellDict)
    runnerShell.SetupShell()
    runnerShell.Run()