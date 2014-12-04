import os
import shutil
import sys
import shlex, subprocess

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'utils', 'python'))
from lm_add_fflux_input import FFluxParameters, SimulationParameter

path = sys.argv[1]
try:
    os.remove('biphasic_switch.lm')
except OSError:
    pass
shutil.copy('wo_fflux.biphasic_switch.lm','biphasic_switch.lm')
replicateParameters = FFluxParameters('biphasic_switch')
simParams = [SimulationParameter(key='crossingsPerPhase',val='100'),
             SimulationParameter(key='maxPhaseZeroTime',val='10000'),
             SimulationParameter(key='maxSteps',val='10000000000'),
             SimulationParameter(key='maxTime',val='1e4'),
             SimulationParameter(key='maxWorkUnitSteps',val='10000'),
             SimulationParameter(key='writeInterval',val='1e3')]
replicateParameters.SetSimulationParameters(simParams=simParams)
replicateParameters.Close()
raw_args = '-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"'
args = shlex.split(raw_args)
p = subprocess.Popen([path] + args)
#os.execv(path, ['-r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"'])
#os.execl(path, '"-r 1,2,3,4,5,6,7,8,9,10"', '-sl', 'lm::cme::GillespieDSolver', '-cr', '1', '-gr', '1/4', '-ff', 'hdf5', '-f', 'biphasic_switch.lm')

#../build/lm -r 1-10 -sl lm::cme::GillespieDSolver -cr 1 -gr 1/4 -ff hdf5 -f "biphasic_switch.lm"