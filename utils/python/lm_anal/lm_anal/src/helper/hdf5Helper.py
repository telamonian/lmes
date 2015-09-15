import h5py

__all__ = ['IsHdf5Dataset', 'IsHdf5Group',
           'RGlobHdf5SubgroupPaths']

# hdf5 object identity test stuff
def IsHdf5Dataset(obj):
    return isinstance(obj, h5py._hl.dataset.Dataset)

def IsHdf5Group(obj):
    return isinstance(obj, h5py._hl.group.Group)

# stuff that overcomes the limitations of the visit/visititems h5py Group methods
def RGlobHdf5SubgroupPaths(group):
    paths = []
    def _RGlobHdf5SubgroupPaths(key, obj):
        if IsHdf5Group(obj):
            paths.append(key)
        return None
    
    group.visititems(_RGlobHdf5SubgroupPaths)
    
    return paths