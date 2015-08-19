#!/usr/bin/env python3
import os,sys
import numpy as np; np.set_printoptions(precision=1, threshold=1e6, linewidth=1e6)
from pathlib import Path
import pytest
import unittest

thisScriptPath = os.path.realpath(__file__)

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
# 
# from lm_anal.test.datum.hist.oparamHists import OParamHistsTestCase
# # 
# from lm_anal.test.datum.oparam.oparams import OParamsTestCase
# 
# from lm_anal.test.datum.parameter.simulationParameter import SimulationParametersTestCase
# 
# from lm_anal.test.datum.tiling.tilings import TilingsTestCase
# 
# from lm_anal.test.datum.trajectory.oparamTrajectories import OParamTrajectoriesTestCase, OParamTrajectoriesClassTestCase
# from lm_anal.test.datum.trajectory.speciesTrajectories import SpeciesTrajectoriesTestCase

# from lm_anal.test.helper.numpyHelper import NumpyHelperTestCase

# from lm_anal.test.io.mod.timeIO import TimeIOTestCase

# from lm_anal.test.io.hdf5.hist.ffluxHistsIO import FFluxHistsHDF5IOTestCase
# from lm_anal.test.io.hdf5.hist.oparamHistsIO import OParamHistsHDF5IOTestCase

# from lm_anal.test.main.sims import SimsTestCase
 
from lm_anal.test.transform.fflux.hist import FFluxOutputToFFluxHistTSimTestCase
# from lm_anal.test.transform.fflux_trajectory.hist.ffluxOutputAndSpeciesTrajectoryToFFluxHistT import FFluxOutputAndSpeciesTrajectoryToFFluxHistTTestCase
# from lm_anal.test.transform.trajectory.hist.speciesTrajectoryToOParamHistT import SpeciesTrajectoryToOParamHistTTestCase

if __name__ == '__main__':
#     FFluxOutputToFFluxHistTSimTestCase.simpleRun()
#     pytest.main(args= thisScriptPath + ' -s')
#     ffluxOutputToFFluxHistTSimTestCase = FFluxOutputToFFluxHistTSimTestCase()
#     ffluxOutputToFFluxHistTSimTestCase.run()
    unittest.main()