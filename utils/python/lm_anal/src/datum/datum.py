from numpy import np

class Datum(object):
    # maps go from hdf5 keys to protoBuf keys
    attrMap = {}
    dataSetMap = {}
    
    def __init__(self, paramBuf, hdf5Group=None):
        self.paramBuf = paramBuf
        
        if hdf5OParamGroup!=None:
            self.InitFromHdf5(hdf5Group)    
        
        for val in self.__class__.attrMap.values():
            self.__setattr__(val, self.paramBuf.__getattribute__(val))
        for val in self.__class__.dataSetMap.values():
            self.__setattr__(val, self.paramBuf.__getattribute__(val))
        
    def InitFromHdf5(self, hdf5Group):
        for key in self.__class__.attrMap.keys():
            self.paramBuf.__setattr__(key, hdf5Group.attrs[key])
        for key in self.__class__.dataSetMap.keys():
            self.__setattr__(key, np.zeros(hdf5Group.shape))
            hdf5Group.read_direct(self.__getattribute__(key))
