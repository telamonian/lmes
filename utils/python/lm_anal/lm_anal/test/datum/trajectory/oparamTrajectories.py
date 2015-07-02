import numpy as np
import os,sys

thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../..'))

from lm_anal.src.io.hdf5.oparam.oparamsIO import OParamsIO
from lm_anal.src.io.hdf5.trajectory.bruteForceTrajectoriesIO import BruteForceTrajectoriesIO
from lm_anal.src.datum.oparam.oparams import OParams
from lm_anal.src.datum.trajectory.oparamTrajectories import OParamTrajectories
from lm_anal.src.datum.trajectory.speciesTrajectories import SpeciesTrajectories
from lm_anal.src.transform.transforms import Transforms
# from lm_anal.src.transform.speciesTrajectoryToOParamTrajectoryT import SpeciesTrajectoryToOParamTrajectoryT as SpecTrajToOPTrajT

import unittest

class OParamTrajectoriesTestCase(unittest.TestCase):
    def setUp(self):
        self.bfTrajIO = BruteForceTrajectoriesIO(fPath=os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm'))
        self.oparamsIO = OParamsIO(fPath=os.path.join(thisScriptDir, '../../testData/biphasic_switch.lm'))
        self.oparams = OParams()
        self.opTraj = OParamTrajectories()
        self.specTraj = SpeciesTrajectories()
    
    def loadData(self, full=False):
        self.bfTrajIO.rff(container=self.specTraj, full=full)
        self.oparamsIO.rff(container=self.oparams, full=full)
        Transforms(src=self.specTraj, dst=self.opTraj, oparam=self.oparams[0])
    
    def test_number_order_parameters_from_transfrom(self):
        '''
        test calculation of number of order parameters (really the sum of the rank of every order parameter) via the reduction of a ReplicateTrajectory
        '''
        self.loadData()
          
        numberOP = self.opTraj[5].number_order_parameters
        self.assertEqual(numberOP, 1)
    
    def test_number_entries_from_transfrom(self):
        '''
        test calculation of number of entries (basically, data rows) via the reduction of a ReplicateTrajectory
        '''
        self.loadData()
        
        numberEntries = self.opTraj[5].number_entries
        self.assertEqual(numberEntries, 101)
    
    def test_order_parameter_values_from_transfrom(self):
        '''
        test calculation of order parameter values via the reduction of a ReplicateTrajectory
        '''
        self.loadData(full=True)
        
        orderParameterValuesArr = np.array(self.opTraj[5].order_parameter_values).flatten()
        intendedOrderParameterValuesArr = np.array([24.0,39.0,34.0,35.0,40.0,45.0,33.0,33.0,35.0,31.0,48.0,45.0,43.0,61.0,47.0,50.0,38.0,33.0,33.0,39.0,39.0,39.0,41.0,41.0,36.0,41.0,38.0,40.0,54.0,37.0,32.0,38.0,32.0,34.0,49.0,52.0,33.0,49.0,49.0,39.0,47.0,42.0,45.0,57.0,33.0,40.0,34.0,46.0,-25.0,-34.0,-35.0,-51.0,-32.0,-44.0,-38.0,-34.0,-37.0,-37.0,-68.0,-37.0,-36.0,-41.0,-40.0,-44.0,-39.0,-38.0,-40.0,-36.0,-49.0,-38.0,-44.0,-48.0,-29.0,-37.0,-19.0,-36.0,-45.0,-36.0,-38.0,-36.0,-35.0,-36.0,-29.0,-27.0,-39.0,-48.0,-41.0,-41.0,-43.0,-46.0,-46.0,-22.0,-37.0,-41.0,-29.0,-38.0,-40.0,-37.0,-35.0,-41.0,-42.0])
        self.assertTrue(np.allclose(orderParameterValuesArr, intendedOrderParameterValuesArr))
    
    def test_time_from_transfrom(self):
        '''
        test calculation of times via the reduction of a ReplicateTrajectory
        '''
        self.loadData(full=True)
        
        timeArr = np.array(self.opTraj[5].time).flatten()
        intendedTimeArr = np.array([0,1000,2000,3000,4000,5000,6000,7000,8000,9000,10000,11000,12000,13000,14000,15000,16000,17000,18000,19000,20000,21000,22000,23000,24000,25000,26000,27000,28000,29000,30000,31000,32000,33000,34000,35000,36000,37000,38000,39000,40000,41000,42000,43000,44000,45000,46000,47000,48000,49000,50000,51000,52000,53000,54000,55000,56000,57000,58000,59000,60000,61000,62000,63000,64000,65000,66000,67000,68000,69000,70000,71000,72000,73000,74000,75000,76000,77000,78000,79000,80000,81000,82000,83000,84000,85000,86000,87000,88000,89000,90000,91000,92000,93000,94000,95000,96000,97000,98000,99000,100000])
        self.assertTrue(np.allclose(timeArr, intendedTimeArr))
        
    def test_trajectory_id_from_transfrom(self):
        '''
        test calculation of trajectory id via the reduction of a ReplicateTrajectory
        '''
        self.loadData()
        
        idInt = self.opTraj[5].trajectory_id
        self.assertEqual(idInt, 5)
