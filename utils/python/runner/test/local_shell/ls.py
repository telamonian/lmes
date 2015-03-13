import os, sys

if __name__ == "__main__" and __package__ is None:
    __package__ = "runner.test.local_shell"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    script_dir_three_up = os.path.split(os.path.split(os.path.split(script_dir)[0])[0])[0]
    sys.path.append(script_dir_three_up)
    import runner.test

from ...job import JobShell
from ...runner import Runner

if __name__=='__main__':
    job_ls_dict = {'arguments': ['>', 'ls.out'],
                   'executable': 'ls',
                   'working_directory': os.getcwd()}
    job_ls = JobShell(**job_ls_dict)
    runner = Runner([job_ls])
    runner.RunAll()