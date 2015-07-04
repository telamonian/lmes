import numpy as np
import os, sys
import unittest

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
testDataPath = os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm')
 
from lm_anal.src.io.hdf5.fflux import FFluxOutputsIO
from lm_anal.src.datum.fflux import FFluxOutputs

class FFluxOutputsTestCase(unittest.TestCase):
    def setUp(self):
        self.ffluxOutsIO = FFluxOutputsIO(fPath=testDataPath)
        self.ffluxOuts = FFluxOutputs()
    
    def loadData(self, full=False):
        self.ffluxOutsIO.rff_Output(container=self.ffluxOuts, full=full)
    
    def test_number_species(self):
        '''
        test the number_species field in FFluxOutput
        '''
        self.loadData()
        
        numberSpecies = self.ffluxOuts[0].number_species
        intendendNumberSpecies = 7
        self.assertEqual(numberSpecies, intendendNumberSpecies)