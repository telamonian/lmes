import numpy as np
import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf'))

from .plottable import Plottable
from TrajectoryState_pb2 import TrajectoryState as TrajectoryStateBuf

class OParamTrajectory(Plottable):
    dataAttr = 'replicateTrajectories'
    
    # properties to support plotting
    @property
    def x(self):
        return self.order_parameter_values[:,0]
    
    # pass through attributes to the underlying TrajectoryStateBuf
    @property
    def number_entries(self):
        return self.trajectoryStateBuf.cme_state.order_parameter_values.number_entries
    @number_entries.setter
    def number_entries(self, val):
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_entries = val
    
    @property
    def number_order_parameters(self):
        return self.trajectoryStateBuf.cme_state.order_parameter_values.number_order_parameters
    @number_order_parameters.setter
    def number_order_parameters(self, val):
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_order_parameters = val
    
    @property
    def trajectory_id(self):
        return self.trajectoryStateBuf.cme_state.order_parameter_values.trajectory_id
    @trajectory_id.setter
    def trajectory_id(self, val):
        self.trajectoryStateBuf.cme_state.order_parameter_values.trajectory_id = val
    
    def __init__(self, sim, full=False, hdf5Group=None, id=None, oparamID=None, repTraj=None):
        self.sim = sim
        self.oparamID = oparamID
        if self.oparamID!=None:
            self.oparam = self.sim.oparams[self.oparamID]
        self.trajectoryStateBuf = TrajectoryStateBuf()
        
        if hdf5Group!=None:
            self.rff(hdf5Group, full=full)
        elif oparamID!=None and repTraj!=None:
            self._transformReplicateTrajectory(repTraj=repTraj, full=full)
    
#         self.number_entries = self.trajectoryStateBuf.cme_state.order_parameter_values.number_entries
#         self.number_order_parameters = self.trajectoryStateBuf.cme_state.order_parameter_values.number_order_parameters
#         self.trajectory_id = self.trajectoryStateBuf.cme_state.order_parameter_values.trajectory_id
    
    def _rff(self, hdf5Group, full=True, useNPArr=True):
        '''
        initialize the data storage container underlying this OParamTrajectoery instance, which is in turn a TrajectoryStateBuf instance (with some numpy arrays thrown in for good measure if useNPArr=True)
        '''
        self.oparamID = hdf5Group.attrs['orderParameterID']
        self.trajectoryStateBuf.trajectory_id = int(hdf5Group.attrs['trajectoryID'])
        self.trajectoryStateBuf.cme_state.order_parameter_values.trajectory_id = int(hdf5Group.attrs['trajectoryID'])
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_entries = hdf5Group['OrderParameterValues'].shape[0]
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_order_parameters = hdf5Group['OrderParameterValues'].shape[1]
        
        if full:
            if useNPArr:
                self._rffArraysNPHDF5(hdf5Group)
            else:
                self._rffArraysBufHDF5(hdf5Group)
                
    def _rffArraysNPHDF5(self, hdf5Group):
        '''
        initializes order_parameter_values and time fields with numpy array based storage
        '''
        self.order_parameter_values = np.zeros(hdf5Group['OrderParameterValues'].shape)
        hdf5Group['OrderParameterValues'].read_direct(self.order_parameter_values)
        self.time = np.zeros(hdf5Group['OrderParameterValueTimes'].shape)
        hdf5Group['OrderParameterValueTimes'].read_direct(self.time)
    
    def _rffArraysBufHDF5(self, hdf5Group):
        '''
        initializes order_parameter_values and time fields with protobuf array based storage
        all of the protobuf stuff is currently 100% python native, so probably worse for very large datasets such as these
        '''
        for val in hdf5Group['OrderParameterValues']:
            self.trajectoryStateBuf.cme_state.order_parameter_values.order_parameter_values.extend(val.tolist())
        for val in hdf5Group['OrderParameterValueTimes']:
            self.trajectoryStateBuf.cme_state.order_parameter_values.time.append(val)
            
        self.order_parameter_values = self.trajectoryStateBuf.cme_state.order_parameter_values.order_parameter_values
        self.time = self.trajectoryStateBuf.cme_state.order_parameter_values.time

    def _transformDatum(self, repTraj):
        self._transformReplicateTrajectory(repTraj=repTraj)
    
    def _transformReplicateTrajectory(self, repTraj, full=True, useNPArr=True):
        '''
        transform the data contained in a ReplicateTrajectory in order to generate this OParamTrajectory's data
        '''
        self.trajectoryStateBuf.trajectory_id = repTraj.trajectory_id
        self.trajectoryStateBuf.cme_state.order_parameter_values.trajectory_id = repTraj.trajectory_id
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_entries = repTraj.number_entries
        self.trajectoryStateBuf.cme_state.order_parameter_values.number_order_parameters = 1
        
        if full:
            if useNPArr:
                self._transformReplicateTrajectoryArraysNP(repTraj)
            else:
                self._transformReplicateTrajectoryArraysBuf(repTraj)
    
    def _transformReplicateTrajectoryArraysBuf(self, repTraj):
        pass
    
    def _transformReplicateTrajectoryArraysNP(self, repTraj):
        self.order_parameter_values = self._transformSpeciesCounts(repTraj.species_count)
        if len(self.order_parameter_values.shape)==1:
            self.order_parameter_values = np.reshape(self.order_parameter_values, (-1,1))
        self.time = np.copy(repTraj.time)
    
    def _transformSpeciesCounts(self, speciesCounts):
        return self.sim.oparams[self.oparamID].calc(speciesCounts)
    
    def _wtf(self, hdf5Group):
        if len(self.order_parameter_values.shape)==1:
            shape = (self.order_parameter_values.shape[0], 1)
        hdf5Group.create_dataset(name='OrderParameterValues', data=self.order_parameter_values, shape=shape)
        hdf5Group.create_dataset(name='OrderParameterValueTimes', data=self.time)
        
        hdf5Group.attrs['orderParameterID'] = self.oparamID
        hdf5Group.attrs['numberEntries'] = self.number_entries
        hdf5Group.attrs['numberOrderParameters'] = self.number_order_parameters
        hdf5Group.attrs['trajectoryID'] = self.trajectory_id