#!/usr/bin/env python3
from inspect import isclass
import os,sys
import numpy as np; np.set_printoptions(precision=1, threshold=1e6, linewidth=1e6)
from pathlib import Path
import pytest
import unittest

thisScriptPath = os.path.realpath(__file__)

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

from lm_anal.test.io.hdf5.trajectory import SpeciesTrajectoriesHDF5IOSimTestBase
from lm_anal.test.io.hdf5.trajectory import OParamTrajectoriesHDF5IOSimTestBase

# from lm_anal.test.io.hdf5.hist.ffluxHistsIO import FFluxHistsHDF5IOSimTestBase
# from lm_anal.test.io.hdf5.hist.oparamHistsIO import OParamHistsHDF5IOTestCase

# from lm_anal.test.main.sims import SimsTestCase
 
# from lm_anal.test.transform.fflux.hist import FFluxOutputToFFluxHistTSimTestBase
# from lm_anal.test.transform.fflux_trajectory.hist.ffluxOutputAndSpeciesTrajectoryToFFluxHistT import FFluxOutputAndSpeciesTrajectoryToFFluxHistTTestCase
# from lm_anal.test.transform.trajectory.hist.speciesTrajectoryToOParamHistT import SpeciesTrajectoryToOParamHistTTestCase
from lm_anal.test.transform.trajectory.trajectory import SpeciesTrajectoryToOParamTrajectoryTSimTestBase

def GetTestBases(varsDict):
    return [TestBase for TestBase in varsDict.values() if isclass(TestBase) and TestBase.__name__[-8:]=='TestBase']

def RunUnittest(varsDict, localsDict, failfast=False, **kwargs):
    for TestBase in GetTestBases(varsDict):
        testCaseName = TestBase.__name__[:-4] + 'Case'
        TestCase = type(testCaseName, (TestBase, unittest.TestCase), {})
        localsDict[testCaseName] = TestCase
    unittest.main(failfast=failfast, **kwargs)

def SimpleRun(varsDict):
    for TestBase in GetTestBases(varsDict):
        testCaseName = TestBase.__name__[:-4] + 'Case'
        TestCase = type(testCaseName, (TestBase, unittest.TestCase), {})
        TestCase.simpleRun()

if __name__ == '__main__':
    RunUnittest(varsDict=vars(), localsDict=locals())
#     SimpleRun(varsDict=vars())
    
    
    
#     pytest.main(args= thisScriptPath + ' -s')