#!/usr/bin/env python3
from main.sim import SimTestCase
from plottable.hist import HistTestCase
from plottable.oparamProbabilityHist import OParamProbabilityHistTestCase
from oparam.oparams import OParamTestCase
from replicate.replicateTrajectories import ReplicateTrajectoriesTestCase
from tiling.tilings import TilingsTestCase

import unittest

if __name__ == '__main__':
    unittest.main()