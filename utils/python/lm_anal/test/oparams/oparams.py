import numpy as np
import os, sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../..'))

import src
from src.oparams.oparams import OParams

import unittest

class OParamTestCase(unittest.TestCase):
    def setUp(self):
        self.oparams = OParams(fPath=os.path.join(thisScriptDir, '../testData/biphasic_switch.lm'))
        
    def test_type_hdf5(self):
        '''
        test reading of oparam type from hdf5 files
        '''
        self.oparams.rffHDF5()

        typeInt = self.oparams[0].type
        self.assertEqal(typeInt, 0)

    def test_id_hdf5(self):
        '''
        test reading of oparam id from hdf5 files
        '''
        self.oparams.rffHDF5()

        idInt = self.oparams[0].id
        self.assertEqal(idInt, 0)

    def test_species_coefficients_hdf5(self):
        '''
        test reading of oparam species coefficient array from hdf5 files
        '''
        self.oparams.rffHDF5()

        speciesCoefficientArr = np.array(self.oparams[0].species_coefficient)
        intendedSpeciesCoefficientArr = np.array([-1.0,-2.0,-2.0,1.0,2.0,2.0])
        self.assertTrue(np.allclose(speciesCoefficientArr, intendedSpeciesCoefficientArr))

    def test_species_ids_hdf5(self):
        '''
        test reading of oparam species id array from hdf5 files
        '''
        self.oparams.rffHDF5()

        speciesIdArr = np.array(self.oparams[0].species_id)
        intendedSpeciesIdArr = np.array([0,1,2,3,4,5])
        self.assertTrue(np.allclose(speciesIdArr, intendedSpeciesIdArr))