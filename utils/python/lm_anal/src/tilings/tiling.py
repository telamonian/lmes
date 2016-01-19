class Tiling(object):
    def __init__(self, tilingBuf, hdf5TilingGroup=None):
        self.tilingBuf = tilingBuf
        if hdf5TilingGroup!=None:
            self.InitFromHdf5(hdf5TilingGroup)

        # add some attributes to the Tiling instance that allow for direct access to the underlying TilingBuf
        self.edges = self.tilingBuf.edges

    def InitFromHdf5(self, hdf5TilingGroup):
            # initialize the data storage container underlying this Tiling instance, which is in turn a TilingBuf instance
            self.tilingBuf.arrangement = 0
            self.tilingBuf.type = hdf5TilingGroup.attrs['Type']
            self.tilingBuf.id = hdf5TilingGroup.attrs['ID']
            self.tilingBuf.order_parameter_id = hdf5TilingGroup.attrs['OrderParameterID']
            self.tilingBuf.type = hdf5TilingGroup.attrs['Type']
            for val in  hdf5TilingGroup['Edges']:
                self.tilingBuf.edges.append(val)