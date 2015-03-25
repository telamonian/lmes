import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.main.sim import Sim

import unittest

class SimTestCase(unittest.TestCase):
    def setUp(self):
        self.oparams = OParams(fPath=os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))