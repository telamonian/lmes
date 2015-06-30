import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../..'))

from lm_anal.src.io.hdf5.trajectory.bruteForceTrajectoriesIO import BruteForceTrajectoriesIO
from lm_anal.src.datum.trajectory.speciesTrajectories import SpeciesTrajectories

import unittest

class SpeciesTrajectoriesTestCase(unittest.TestCase):
    def setUp(self):
        self.bfTrajIO = BruteForceTrajectoriesIO(fPath=os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm'))
        self.specTraj = SpeciesTrajectories()
    
    def test_number_species_hdf5(self):
        '''
        test reading of number of species(s) from hdf5 files
        '''
        self.bfTrajIO.rff(container=self.specTraj)

        numberSpecies = self.specTraj[4].number_species
        self.assertEqual(numberSpecies, 7)
    
    def test_number_entries_hdf5(self):
        '''
        test reading of number of entries (basically, data rows) from hdf5 files
        '''
        self.bfTrajIO.rff(container=self.specTraj)

        numberEntries = self.specTraj[4].number_entries
        self.assertEqual(numberEntries, 101)
    
    def test_species_count_hdf5(self):
        '''
        test reading of species counts from hdf5 files
        '''
        self.bfTrajIO.rff(container=self.specTraj, full=True)

        speciesCountArr = np.array(self.specTraj[4].species_count).flatten()
        intendedSpeciesCountArr = np.array([0,0,0,16,4,0,1,0,0,0,4,13,1,0,0,0,0,3,16,1,0,0,0,0,6,20,1,0,0,0,0,2,9,1,0,0,0,0,3,13,1,0,0,0,0,6,12,1,0,0,0,0,3,21,1,0,0,0,0,3,20,1,0,0,0,0,4,18,1,0,0,0,0,4,16,1,0,0,0,0,2,18,1,0,0,0,0,5,11,1,0,0,0,0,4,13,1,0,0,0,0,2,22,1,0,0,0,0,9,13,1,0,0,0,0,5,18,1,0,0,0,0,4,16,1,0,0,0,0,4,20,1,0,0,0,0,3,18,1,0,0,0,0,5,24,1,0,0,0,0,2,10,1,0,0,0,0,3,20,1,0,0,0,0,3,23,1,0,0,0,0,4,17,1,0,0,0,0,8,10,1,0,0,0,0,4,15,1,0,0,0,0,3,19,1,0,0,0,0,3,16,1,0,0,0,0,3,12,1,0,0,0,0,4,21,1,0,0,0,0,4,11,1,0,0,0,0,10,16,1,0,0,0,0,4,21,1,0,0,0,0,6,16,1,0,0,0,0,0,17,1,0,0,0,0,5,18,1,0,0,0,0,4,17,1,0,0,0,0,5,8,1,0,0,0,0,2,11,1,0,0,0,0,3,14,1,0,0,0,0,2,20,1,0,0,0,0,4,12,1,0,0,0,0,7,17,1,0,0,0,0,5,20,1,0,0,0,0,2,13,1,0,0,0,0,4,17,1,0,0,0,0,2,19,1,0,0,0,0,6,10,1,0,0,0,0,5,10,1,0,0,0,0,2,18,1,0,0,0,0,1,23,1,0,0,0,0,5,16,1,0,0,0,0,4,12,1,0,1,0,0,3,11,1,0,0,0,0,3,18,1,0,0,0,0,3,18,1,0,0,0,0,4,10,1,0,0,0,0,7,11,1,0,0,0,0,3,11,1,0,0,0,0,1,15,1,0,0,0,0,4,10,1,0,0,0,0,5,14,1,0,0,0,0,4,24,1,0,0,0,0,6,21,1,0,0,0,0,0,10,1,0,0,0,0,3,13,1,0,0,0,0,4,16,1,0,0,0,0,4,15,1,0,0,0,0,3,11,1,0,0,0,0,4,16,1,0,0,0,0,2,10,1,0,0,0,0,6,25,1,0,0,0,0,3,15,1,0,0,0,0,5,13,1,0,0,0,0,3,19,1,0,0,0,0,2,18,1,0,0,0,0,6,23,1,0,0,0,0,3,13,1,0,0,0,0,3,11,1,0,0,0,0,1,15,1,0,0,0,0,7,15,1,0,0,0,0,2,18,1,0,0,0,0,5,21,1,0,0,0,0,3,14,1,0,0,0,0,3,14,1,0,0,0,0,6,10,1,0,0,0,0,5,16,1,0,0,0,0,4,20,1,0,0,0,0,4,19,1,0,0,0,0,6,22,1,0,0,0,0,3,16,1,0,0,0,0,4,14,1,0,0,0,0,3,17,1,0,0,0,0,4,16,1,0,0,0,0,2,12,1,0,0,0,0,2,15,1,0,0,0,0,0,21,1,0,0,0,0,5,13,1,0,0,0,0,3,13,1,0,0,0,0,3,18,1,0])
        self.assertTrue(np.allclose(speciesCountArr, intendedSpeciesCountArr))
    
    def test_time_hdf5(self):
        '''
        test reading of times from hdf5 files
        '''
        self.bfTrajIO.rff(container=self.specTraj, full=True)

        timeArr = np.array(self.specTraj[4].time).flatten()
        intendedTimeArr = np.array([0,1000,2000,3000,4000,5000,6000,7000,8000,9000,10000,11000,12000,13000,14000,15000,16000,17000,18000,19000,20000,21000,22000,23000,24000,25000,26000,27000,28000,29000,30000,31000,32000,33000,34000,35000,36000,37000,38000,39000,40000,41000,42000,43000,44000,45000,46000,47000,48000,49000,50000,51000,52000,53000,54000,55000,56000,57000,58000,59000,60000,61000,62000,63000,64000,65000,66000,67000,68000,69000,70000,71000,72000,73000,74000,75000,76000,77000,78000,79000,80000,81000,82000,83000,84000,85000,86000,87000,88000,89000,90000,91000,92000,93000,94000,95000,96000,97000,98000,99000,100000])
        self.assertTrue(np.allclose(timeArr, intendedTimeArr))
        
    def test_trajectory_id_hdf5(self):
        '''
        test reading of trajectory id from hdf5 files
        '''
        self.bfTrajIO.rff(container=self.specTraj)

        idInt = self.specTraj[4].trajectory_id
        self.assertEqual(idInt, 4)
        
        idInt = self.specTraj[8].trajectory_id
        self.assertEqual(idInt, 8)
