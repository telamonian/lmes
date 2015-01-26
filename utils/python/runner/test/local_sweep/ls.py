import os
import sys

thisScriptsPath = os.path.dirname(os.path.realpath(__file__))

host = 'localhost'
lm_bin = '/Users/tel/git/lm/build/lm'
local_home_directory = thisScriptsPath
remote_home_directory = 'Users/tel'
type = 'shell'
user_id = 'tel'

if __name__ == "__main__" and __package__ is None:
    __package__ = "runner.test.remote_sge"
    script_dir = os.path.dirname(os.path.abspath(__file__))
    script_dir_three_up = os.path.split(os.path.split(os.path.split(script_dir)[0])[0])[0]
    sys.path.append(script_dir_three_up)
    import runner.test

from ...helper import *
from ...job import JobSGE
from ...runner import Runner
from ...sweep import ReactionRateConstant, Sweep, SweepTup

if __name__=='__main__':
    xTicks = LogTicks(-2,-1,base=10,resolution=0)
    yTicks = xTicks/4.0
    # production reactions get swept through in x...
    inputTupssX = [[ReactionRateConstant(reactionID=4, rateConstant=tick), ReactionRateConstant(reactionID=5, rateConstant=tick), ReactionRateConstant(reactionID=11, rateConstant=tick), ReactionRateConstant(reactionID=12, rateConstant=tick)] for tick in xTicks]
    # ...and degradation reactions get swept through in y
    inputTupssY = [[ReactionRateConstant(reactionID=6, rateConstant=tick), ReactionRateConstant(reactionID=13, rateConstant=tick)] for tick in yTicks]
    sweepTupX = SweepTup(inputTupss=inputTupssX, label='production%.5f', labelVals=xTicks)
    sweepTupY = SweepTup(inputTupss=inputTupssY, label='degradation%.5f', labelVals=yTicks)
    
    sweep_dict = {'cpu_count': 2, 
                  'host': host, 
                  'lm_bin': lm_bin,
                  'lm_file_path': 'biphasic_switch.lm', 
                  'rootPath': os.path.join(remote_home_directory, 'test/sweep'), 
                  'sweepTupX': sweepTupX, 
                  'sweepTupY': sweepTupY, 
                  'type': type, 
                  'user_id': user_id}
    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()