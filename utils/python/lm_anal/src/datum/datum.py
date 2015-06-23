import numpy as np

DEBUG_GETTERS_SETTERS = False

class DatumMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        if 'propertySpecs' in dct:
            for name,val in dct['propertySpecs'].items():
                if val['type']=='scalar':
                    if val['storageType']=='protoBuf':
                        getterList = ['@property',
                                      'def %s(self):' % name,
                                      '\treturn self.protoBuf.%s' % '.'.join(val['paths'])]
                        setterList = ['@%s.setter' % name,
                                      'def %s(self, val):' % name,
                                      '\tself.protoBuf.%s = val' % '.'.join(val['paths'])]
                        extraList = ['dct[name] = %s' % name]
                elif val['type']=='array':
                    if val['storageType']=='numpy':
                        pass
                    elif val['storageType']=='protoBuf':
                        getterList = ['@property',
                                      'def %s(self):' % name,
                                      '\treturn self.protoBuf.%s' % '.'.join(val['paths'])]
                        setterList = ['@%s.setter' % name,
                                      'def %s(self, val):' % name,
#                                       '\ttry:'
                                      '\tself.protoBuf.%s.extend(val)' % '.'.join(val['paths'])]
#                                       '\texcept AttributeError:',
#                                       '\t\tself.protoBuf.%s.extend(val.resize)' % '.'.join(val['paths'])]
                        extraList = ['dct[name] = %s' % name]
                if DEBUG_GETTERS_SETTERS:
                    # list[-1:-1] = [otherList] inserts the elements of otherList in front of the final element of list
                    getterList[-1:-1] = ["\tprint('getter for the %s property was called')" % name]
                    setterList[-1:-1] = ["\tprint('setter for the %s property was called')" % name]
                exec('\n'.join(getterList + setterList + extraList))
        return super(DatumMetaclass, cls).__new__(cls, clsname, bases, dct)

class Datum(object, metaclass=DatumMetaclass):
    # maps go from hdf5 keys to protoBuf keys
#     attrMap = {}
#     dataSetMap = {}
    
    # make sure that these are tuples and not some mutable data type. Otherwise, inheritance hell
    arrays = None
    scalars = None
    
    def __init__(self, full=True):
        # has this datum been made from a full set of input, or only a partial one (eg without arrays)?
        self.full = full
    
    def getArray(self, name, dims=None, dtype=None):
        try:
            return self.__getattribute__(name)
        except AttributeError:
            self.__setattr__(name, np.zeros(dims, dtype=dtype))
            return self.__getattribute__(name)
    
    def getScalar(self, name):
        return self.__getattribute__(name)
    
    def setArray(self, name, val):
        self.__setattr__(name, val)
    
    def setScalar(self, name, val):
        self.__setattr__(name, val)