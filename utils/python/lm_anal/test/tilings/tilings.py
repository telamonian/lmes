import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.tilings.tilings import Tilings

import unittest

class TilingsTestCase(unittest.TestCase):
    def setUp(self):
        self.tilings = Tilings(fPath=os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))
        
    def test_edges_hdf5(self):
        self.tilings.rffHDF5()

        # test reading of tilings from hdf5 and storing them with integer type keys
        edgeArr = np.array(self.tilings[19].edges)
        intendedEdgeArr = np.linspace(-25,25,13)
        self.assertTrue(np.allclose(edgeArr, intendedEdgeArr))

        edgeArr = np.array(self.tilings[199].edges)
        intendedEdgeArr = np.linspace(-30,30,16)
        self.assertTrue(np.allclose(edgeArr, intendedEdgeArr))