import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../..'))
 
# import src
# from src.oparam.oparams import OParams

from src.io.hdf5.oparam.oparamsIO import OParamsIO
from src.datum.oparam.oparams import OParams

class OParamsTestCase(unittest.TestCase):
    def setUp(self):
        self.oparamsIO = OParamsIO(fPath=os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm'))
        self.oparams = OParams()
        self.oparamsIO.rff(container=self.oparams)
        
    def test_calc(self):
        '''
        test calculation of oparam value from species vector
        '''    
        oparamVal = self.oparams[0].calc(np.array(((4,16,1,0,0,0,0),)))
        intendedOParamVal = -38
        self.assertEqual(oparamVal, intendedOParamVal)
        
        oparamVal = self.oparams[1].calc(np.array(((4,16,1,0,0,0,0),)))
        intendedOParamVal = 38
        self.assertEqual(oparamVal, intendedOParamVal)

        oparamVal = self.oparams[2].calc(np.array(((74,0,13,4,16,1,0),)))
        intendedOParamVal = 38
        self.assertEqual(oparamVal, intendedOParamVal)

        oparamVal = self.oparams[2].calc(np.array(((74,0,13,1,0,0,0),)))
        intendedOParamVal = 1
        self.assertEqual(oparamVal, intendedOParamVal)

    def test_has(self):
        '''
        test detection of presence relevant data in hdf5 files
        '''
        self.assertTrue(self.oparamsIO.has())

    def test_id_hdf5(self):
        '''
        test reading of oparam id from hdf5 files
        '''
        idInt = self.oparams[0].id
        self.assertEqual(idInt, 0)

    def test_species_coefficients_hdf5(self):
        '''
        test reading of oparam species coefficient array from hdf5 files
        '''
        speciesCoefficientArr = np.array(self.oparams[0].species_coefficients)
        intendedSpeciesCoefficientArr = np.array([-1.0,-2.0,-2.0,1.0,2.0,2.0])
        self.assertTrue(np.allclose(speciesCoefficientArr, intendedSpeciesCoefficientArr))

    def test_species_ids_hdf5(self):
        '''
        test reading of oparam species id array from hdf5 files
        '''
        speciesIdArr = np.array(self.oparams[0].species_ids)
        intendedSpeciesIdArr = np.array([0,1,2,3,4,5])
        self.assertTrue(np.allclose(speciesIdArr, intendedSpeciesIdArr))
        
    def test_type_hdf5(self):
        '''
        test reading of oparam type from hdf5 files
        '''
        typeInt = self.oparams[0].type
        self.assertEqual(typeInt, 0)