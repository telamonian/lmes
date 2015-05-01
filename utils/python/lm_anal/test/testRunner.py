#!/usr/bin/env python3
from main.sim import SimTestCase
from plotable.hist import HistTestCase
from plotable.oparamProbabilityHist import OParamProbabilityHistTestCase
from oparam.oparams import OParamTestCase
from replicate.replicateTrajectories import ReplicateTrajectoriesTestCase
from tiling.tilings import TilingsTestCase

import unittest

if __name__ == '__main__':
    unittest.main()