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


from test.datum.trajectory.speciesTrajectories import SpeciesTrajectoriesTestCase



if __name__ == '__main__':
    unittest.main()