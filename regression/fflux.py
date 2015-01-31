#!/usr/bin/env python

import os
import shutil
import sys

lmBinPath = sys.argv[1]
try:
    os.remove('biphasic_switch.lm')
except OSError:
    pass
shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')
os.system('../utils/python/lm_add_fflux_input.py biphasic_switch.lm')
os.execl(lmBinPath, '-r 1-10', '-sl', 'lm::cme::GillespieDSolver', '-cr', '1', '-gr', '1/4', '-ff', 'hdf5', '-fflux', '-f', 'biphasic_switch.lm')

#../build/lm -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -fflux -f "biphasic_switch.lm"
