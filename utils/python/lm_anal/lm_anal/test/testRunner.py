#!/usr/bin/env python3
import os,sys
sys.path = ['..'] + sys.path
import unittest

# from main.sim import SimTestCase
# from main.sim import SimCreateOParamHistTestCase
# from main.sim import SimCreateOParamTrajectoryTestCase
# from plottable.hist import HistTestCase
# from plottable.oparamProbabilityHist import OParamProbabilityHistTestCase
# from plottable.oparamTrajectory import OParamTrajectoryTransformTestCase
# from oparam.oparams import OParamTestCase
#from replicate.replicateTrajectories import ReplicateTrajectoriesTestCase
# from tiling.tilings import TilingsTestCase

# from lm_anal.test.datum.fflux.ffluxBasins import FFluxBasinsTestCase
# from lm_anal.test.datum.fflux.ffluxFinals import FFluxFinalsTestCase
# from lm_anal.test.datum.fflux.ffluxOutputs import FFluxOutputsTestCase
# from lm_anal.test.datum.fflux.ffluxTrajectories import FFluxTrajectoriesTestCase

from lm_anal.test.datum.hist.oparamHists import OParamHistsTestCase
# 
# from lm_anal.test.datum.oparam.oparams import OParamsTestCase
#  
# from lm_anal.test.datum.parameter.simulationParameter import SimulationParametersTestCase
#  
# from lm_anal.test.datum.tiling.tilings import TilingsTestCase
#  
# from lm_anal.test.datum.trajectory.oparamTrajectories import OParamTrajectoriesTestCase
# from lm_anal.test.datum.trajectory.speciesTrajectories import SpeciesTrajectoriesTestCase
# 
# from lm_anal.test.transform.ffluxOutputToOParamHistT import FFluxOutputToOParamHistTTestCase

if __name__ == '__main__':
    unittest.main()