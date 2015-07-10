ASCENDING = 0
DESCENDING = 1

from lm_anal.src.datum.datum import Datum
from lm_anal.src.datum.datum import DatumMetaclass

class Tiling(Datum, metaclass=DatumMetaclass):
    propertySpecs = {'arrangement':{'dtype':'int', 'paths':('arrangement',), 'storageType':'protobuf', 'type':'array'},
                 'dims':{'dtype':'int', 'paths':('dims',), 'storageType':'protobuf', 'type':'array'},
                 'edges':{'dtype':'float', 'paths':('edges',), 'storageType':'protobuf', 'type':'array'},
                 'id':{'dtype':'int', 'paths':('id',), 'storageType':'protobuf', 'type':'scalar'},
                 'order_parameter_id':{'dtype':'int', 'paths':('order_parameter_id',), 'storageType':'protobuf', 'type':'scalar'},
                 'rank':{'dtype':'int', 'paths':('rank',), 'storageType':'protobuf', 'type':'scalar'},
                 'type':{'dtype':'int', 'paths':('type',), 'storageType':'protobuf', 'type':'scalar'}}
    
    def __init__(self, subcon, full=False):
        super().__init__(full=full)
        self.protobuf = subcon
        
    @property
    def name(self):
        try:
            return self._name
        except AttributeError:
            return self.id
    @name.setter
    def name(self, val):
        self._name = val
    
#     def __init__(self, tilingBuf, hdf5TilingGroup=None):
#         self.protobuf = tilingBuf
#         
#         # add some attributes to the Tiling instance that allow for direct access to the underlying TilingBuf
#         self.arrangement = self.tilingBuf.arrangement
#         self.id = self.tilingBuf.id
#         self.order_parameter_id = self.tilingBuf.order_parameter_id
#         self.type = self.tilingBuf.type
#         self.edges = self.tilingBuf.edges
#         
#         self.rank = self.tilingBuf.rank
#         self.dims = self.tilingBuf.dims
#         
#         if hdf5TilingGroup!=None:
#             self.InitFromHdf5(hdf5TilingGroup)
#         self.InitArrangement()

#     def InitArrangement(self):
#         '''
#         run this after the rest of the data has been loaded into self.tilingBuf
#         '''
#         
#         # based on first and last edges, infer if this tiling is arranged ASCENDING=0 or DESCENDING=1
#         self.arrangement = ASCENDING if self.edges[-1]>=self.edges[0] else DESCENDING
#         
#         # check the sorting of the edges to make sure our guess is correct
#         if self.arrangement==ASCENDING:
#             for edge,nextEdge in zip(self.edges[:-1],self.edges[1:]):
#                 if edge > nextEdge:
#                     raise ValueError
#         else: # if self.arrangement==DESCENDING:
#             for edge,nextEdge in zip(self.edges[:-1],self.edges[1:]):
#                 if nextEdge > edge:
#                     raise ValueError
# 
#     def InitFromHdf5(self, hdf5TilingGroup):
#             # initialize the data storage container underlying this Tiling instance, which is in turn a TilingBuf instance
#             self.type = int(hdf5TilingGroup.attrs['Type'])
#             self.id = int(hdf5TilingGroup.attrs['ID'])
#             self.order_parameter_id = int(hdf5TilingGroup.attrs['OrderParameterID'])
#             self.edges.extend(hdf5TilingGroup['Edges'][...].tolist())
# #             for val in hdf5TilingGroup['Edges']:
# #                 self.tilingBuf.edges.append(val)
#             
#             self.rank = 1
#             self.dims.append(len(self.edges) + 1)
    