import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

host = 'xanthus'
local_home_directory = thisScriptsPath
remote_home_directory = 'home/cklein13'
user_id = 'cklein13'



if __name__ == "__main__" and __package__ is None:
    __package__ = "runner.test.remote_sge"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    script_dir_three_up = os.path.split(os.path.split(os.path.split(script_dir)[0])[0])[0]
    sys.path.append(script_dir_three_up)
    import runner.test

from ...job import JobSGE
from ...runner import Runner



if __name__=='__main__':
    job_ls_dict = {'arguments': ['ls.sh'],
                   'copy_from': [(os.path.join(remote_home_directory, 'test','ls.sgeout'), local_home_directory)],
                   'copy_to': [(os.path.join(local_home_directory,'ls.sh'), os.path.join(remote_home_directory, 'test'))],
                   'executable': '/bin/bash',
                   'host': host,
                   'output': 'ls.sgeout',
                   'user_id': user_id,
                   'working_directory': os.path.join('$HOME','test')}
    job_ls = JobSGE(**job_ls_dict)
    print job_ls.host
    runner = Runner([job_ls])
    runner.RunAll()