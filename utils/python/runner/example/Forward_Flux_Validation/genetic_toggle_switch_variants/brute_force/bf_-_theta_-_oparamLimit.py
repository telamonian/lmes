#!/usr/bin/env python

import os,sys
thisScriptsPath = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptsPath, '..'))
from paramSetup import GenBarrierHeightSweepTup, GenBFDefaultInputTup, \
                       GenFPTDefaultInputTup, GenOParamLimitDefaultInputTupFromBasin

#### USER DEFINED VARIABLES ####
starting_basin = 'b'

# host = 'kirin'
# lm_bin = '/home/erober32/usr/bin/lmes'      #'/home/cklein13/git/lm/build/lmes'
# remote_home_directory = '/home/cklein13'
# jobTypeName = 'sge'
# user_id = 'cklein13'
# pass_exe = None
# user_mail = None

host = 'gateway2.marcc.jhu.edu'
lm_bin = '/home-1/cklein13@jhu.edu/git/lm/build/lmes'   #'/home-2/erober32@jhu.edu/usr/bin/lmes'
remote_home_directory = '/home-1/cklein13@jhu.edu/work/cklein13'
remote_relative_dir = 'forward_flux_validation/name_gts_-_sim_bf_-_oparamLimit_27_-_samples_None_-_basin_%s' % starting_basin.upper()
jobTypeName = 'slurm'
user_id = 'cklein13@jhu.edu'
# pass_exe = '/Users/tel/usr/bin/sp_marcc'
user_mail = 'cklein13@jhu.edu'

local_home_directory = thisScriptsPath
################################

from helper import *
from sweep import Sweep

def GenDefaultInputTups(basin):
    return GenBFDefaultInputTup(basin=basin) + \
           GenFPTDefaultInputTup(speciesIDs=[0,1,2,3,4,5,6]) + \
           GenOParamLimitDefaultInputTupFromBasin(basin=basin)

def GenSweepTups():
    thetaTicks = LogTicks(1,-1,base=10,resolution=1)

    return [GenBarrierHeightSweepTup(ticks=thetaTicks)]

if __name__=='__main__':
    sweep_dict = {'cpu_count': 24*42,
#                   'diagonal': True,
                  'host': host,
                  'inputTupsDefault': GenDefaultInputTups(basin=starting_basin),
                  'lm_bin': lm_bin,
                  'lm_file_path': 'genetic_toggle_switch.lm',
                  'lm_sampling_rate': 1e11,    #{'rate':'auto', 'weight':.1}, #1e3
                  'lm_sampling_time': 1e10,
                  'replicateRange': (1,1000),
                  'rootPath': PathJoin(remote_home_directory, remote_relative_dir),
                  'sweepTups': GenSweepTups(),
                  'jobTypeName': jobTypeName,
                  'user_id': user_id}

    try:
        sweep_dict['queue'] = queue
        if 'gpu' in queue:
            sweep_dict['lmArgsGpusPerReplicate'] = '1/4'
    except NameError:
        pass

    if host=='gateway2.marcc.jhu.edu':
        sweep_dict.update({#'pass_exe': pass_exe,
                           'user_mail': user_mail})

    sweep = Sweep(**sweep_dict) 
    sweep.Setup()
    sweep.Run()
