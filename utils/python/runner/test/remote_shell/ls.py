host = 'xanthus'
user_id = 'cklein13'

import os, sys

if __name__ == "__main__" and __package__ is None:
    __package__ = "runner.test.remote_shell"    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    script_dir_three_up = os.path.split(os.path.split(os.path.split(script_dir)[0])[0])[0]
    sys.path.append(script_dir_three_up)
    import runner.test

from ...job import JobShell
from ...runner import Runner

if __name__=='__main__':
    runner = Runner()
    job_ls_dict = {'arguments': ['|', 'tee', 'ls.out'],
                   'executable': 'ls',
                   'host': host,
                   'output': 'ls.stdout',
                   'user_id': user_id,
                   'working_directory': os.path.join('$HOME','test')}
    job_ls = JobShell(**job_ls_dict)
    job_ls.SetRunner(runner)
    runner.Setup()
    runner.Run()