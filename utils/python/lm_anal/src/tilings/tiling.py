ASCENDING = 0
DESCENDING = 1

class Tiling(object):
    def __init__(self, tilingBuf, hdf5TilingGroup=None):
        self.tilingBuf = tilingBuf
        if hdf5TilingGroup!=None:
            self.InitFromHdf5(hdf5TilingGroup)

        # add some attributes to the Tiling instance that allow for direct access to the underlying TilingBuf
        self.arrangement = self.tilingBuf.arrangement
        self.id = self.tilingBuf.id
        self.order_parameter_id = self.tilingBuf.order_parameter_id
        self.type = self.tilingBuf.type
        self.edges = self.tilingBuf.edges
        
        self.InitArrangement()

    def InitArrangement(self):
        '''
        run this after the rest of the data has been loaded into self.tilingBuf
        '''
        
        # based on first and last edges, infer if this tiling is arranged ASCENDING=0 or DESCENDING=1
        self.arrangement = ASCENDING if self.edges[-1]>=self.edges[0] else DESCENDING
        
        # check the sorting of the edges to make sure our guess is correct
        if self.arrangement==ASCENDING:
            for edge,nextEdge in zip(self.edges[:-1],self.edges[1:]):
                if edge > nextEdge:
                    raise ValueError
        else: # if self.arrangement==DESCENDING:
            for edge,nextEdge in zip(self.edges[:-1],self.edges[1:]):
                if nextEdge > edge:
                    raise ValueError

    def InitFromHdf5(self, hdf5TilingGroup):
            # initialize the data storage container underlying this Tiling instance, which is in turn a TilingBuf instance
            self.tilingBuf.type = int(hdf5TilingGroup.attrs['Type'])
            self.tilingBuf.id = int(hdf5TilingGroup.attrs['ID'])
            self.tilingBuf.order_parameter_id = int(hdf5TilingGroup.attrs['OrderParameterID'])
            for val in hdf5TilingGroup['Edges']:
                self.tilingBuf.edges.append(val)
    