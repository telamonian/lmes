import numpy as np
import os

from .hist import Hist

class OParamProbabilityHist(Hist):
    dataAttr = 'replicateTrajectories'
    
    def __init__(self, sim, id=None, tilingID=None):
        if id!=None:
            self.id = id
        self.sim = sim
        self.oparams = sim.oparams
        self.tilingID = tilingID
    
    def Init(self):
        if self.tilingID==None:
            raise
        self.tiling = self.sim.tilings[self.tilingID]
        dims = self.tiling.dims
        edges = self.tiling.edges
        rank = self.tiling.rank
        Hist.__init__(self,dims,edges,rank)
        self.InitEdgesNames()
    
    def InitEdgesNames(self):
        # edgesNames is used for naming the hdf5 datasets that contain the edge values for a particular dimension
        if self.rank < 5:
            self.edgesNames = ('x','y','z','w')
        else:
            self.edgesNames = range(self.rank)
    
    def _transformDatum(self, datum):
        self.AddSpeciesCounts(datum.species_count)
    
    def _rff(self, hdf5Group):
        dims = hdf5Group.attrs['dims']
        rank = hdf5Group.attrs['rank']
        self.tilingID = hdf5Group.attrs['tilingID']
        self.tiling = self.sim.tilings[self.tilingID]
        edges = np.zeros((np.sum(dims - 1)))
        i = 0
        for i,key in enumerate(sorted(hdf5Group['edges'])):
            hdf5Group['edges'][key].read_direct(edges, dest_sel=np.s_[i:i+hdf5Group['edges'][key].size])
            i+=hdf5Group['edges'][key].size
        Hist.__init__(self,dims,edges,rank)
        hdf5Group['vals'].read_direct(self.vals)
        self.InitEdgesNames()
            
    def _wtf(self, hdf5Group):
        hdf5Group.create_dataset(name='vals', data=self.vals)
        hdf5EdgesGroup = hdf5Group.create_group('edges')
        for name, edges in zip(self.edgesNames, self.GetEdges()):
            hdf5EdgesGroup.create_dataset(name=str(name), data=np.array(edges))
        hdf5Group.attrs['dims'] = np.array(self.dims)
        hdf5Group.attrs['rank'] = self.rank
        hdf5Group.attrs['tilingID'] = self.tilingID

    def AddSpeciesCounts(self, speciesCounts):
        oparamVals = self.oparams[self.tiling.order_parameter_id].calc(speciesCounts)
        self.AddObs(oparamVals)