import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.tiling.tilings import Tilings

import unittest

class TilingsTestCase(unittest.TestCase):
    def setUp(self):
        self.tilings = Tilings(fPath=os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))
        self.tilings.rffHDF5()
    
    def test_arrangement(self):
        '''
        test determination of arrangement of edges
        '''
        arrangementInt = self.tilings[19].arrangement
        self.assertEqual(arrangementInt, 0)
        
        arrangementInt = self.tilings[199].arrangement
        self.assertEqual(arrangementInt, 0)
        
    def test_edges_hdf5(self):
        '''
        test reading of tiling edges from hdf5 files
        '''
        edgeArr = np.array(self.tilings[19].edges)
        intendedEdgeArr = np.linspace(-25,25,13)
        self.assertTrue(np.allclose(edgeArr, intendedEdgeArr))

        edgeArr = np.array(self.tilings[199].edges)
        intendedEdgeArr = np.linspace(-30,30,16)
        self.assertTrue(np.allclose(edgeArr, intendedEdgeArr))
    

    def test_id_hdf5(self):
        '''
        test reading of tiling id from hdf5 files
        '''
        idInt = self.tilings[19].id
        self.assertEqual(idInt, 19)
        
        idInt = self.tilings[199].id
        self.assertEqual(idInt, 199)
        
    def test_order_parameter_id_hdf5(self):
        '''
        test reading of the id of the tiling's order parameter from hdf5 files
        '''
        oparamIDInt = self.tilings[19].order_parameter_id
        self.assertEqual(oparamIDInt, 0)
        
        oparamIDInt = self.tilings[199].order_parameter_id
        self.assertEqual(oparamIDInt, 0)
    
    def test_type_hdf5(self):
        '''
        test reading of tiling type from hdf5 files
        '''
        typeInt = self.tilings[19].type
        self.assertEqual(typeInt, 0)

        typeInt = self.tilings[199].type
        self.assertEqual(typeInt, 0)
