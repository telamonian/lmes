from copy import deepcopy
import numpy as np
import re

DEBUG_GETTERS_SETTERS = False

def DefAliasProp(name, spec):
    @property
    def prop(self):
        return self.__getattribute__(spec['targetName'])
    @prop.setter
    def prop(self, val):
        self.__setattr__(spec['targetName'], val)
    return prop

def DefNPProp(name, spec):
    @property
    def prop(self):
        return self.__getattribute__('_'+name)
    @prop.setter
    def prop(self, val):
        if hasattr(val, 'read_direct') and callable(getattr(val, 'read_direct', None)):
            # initialize the array if it doesn't already exist
            self.getArray(dims=val.shape, dtype=spec['dtype'], name='_'+name)
            val.read_direct(self.__getattribute__('_'+name))
        else:
            self.__setattr__('_'+name, val)
    return prop

class DatumMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        _propertyNames = dct.get('_propertyNames', set())
        if 'propertySpecs' in dct:
            for name,val in dct['propertySpecs'].items():
                _propertyNames.add(name)
                if val['type']=='scalar':
                    if val['storageType']=='protobuf':
                        getterList = ['@property',
                                      'def %s(self):' % name,
                                      '\treturn self.protobuf.%s' % '.'.join(val['paths'])]
                        setterList = ['@%s.setter' % name,
                                      'def %s(self, val):' % name,
                                      '\tself.protobuf.%s = %s(val)' % ('.'.join(val['paths']), val['dtype'])]
                        extraList = ['dct[name] = %s' % name]
                elif val['type']=='array':
                    if val['storageType']=='numpy':
                        dct[name] = DefNPProp(name, val)
                        continue
                    elif val['storageType']=='protobuf':
                        getterList = ['@property',
                                      'def %s(self):' % name,
                                      '\treturn self.protobuf.%s' % '.'.join(val['paths'])]
                        setterList = ['@%s.setter' % name,
                                      'def %s(self, val):' % name,
                                      '\ttry:',
                                      '\t\tself.protobuf.%s.extend(val.astype(%s).flatten().tolist())' % ('.'.join(val['paths']), val['dtype']),
                                      '\texcept AttributeError:',
                                      '\t\ttmpArr=np.zeros(val.shape, dtype=%s)' % val['dtype'],
                                      '\t\tval.read_direct(tmpArr)',
                                      '\t\tself.protobuf.%s.extend(tmpArr.flatten().tolist())' % '.'.join(val['paths'])]
                        extraList = ['dct[name] = %s' % name]
                elif val['type']=='alias':
                    dct[name] = DefAliasProp(name, val)
                    continue 
                if DEBUG_GETTERS_SETTERS:
                    # list[-1:-1] = [otherList] inserts the elements of otherList in front of the final element of list
                    indent = re.match('(\t*)', getterList[-1]).group(1)
                    getterList[-1:-1] = ["%sprint('getter for the %s property was called')" % (indent, name)]
                    indent = re.match('(\t*)', setterList[-1]).group(1)
                    setterList[-1:-1] = ["%sprint('setter for the %s property was called')" % (indent, name)]
                exec('\n'.join(getterList + setterList + extraList))
        dct['_propertyNames'] = _propertyNames
        return super(DatumMetaclass, cls).__new__(cls, clsname, bases, dct)
    
    @property
    def propertyNames(cls):
        return super(cls,cls)._propertyNames | cls._propertyNames
    
class Datum(object, metaclass=DatumMetaclass):
    # maps go from hdf5 keys to protobuf keys
#     attrMap = {}
#     dataSetMap = {}
    
    # make sure that these are tuples and not some mutable data type. Otherwise, inheritance hell
    arrays = None
    scalars = None
    
    def __init__(self, full=True):
        # full: has this datum been made from a full set of input, or only a partial one (eg without arrays)?
        self.full = full
    
    def initEmbedded(self, name, DataType):
        self.__setattr__(name, DataType(self.protobuf))
        return self.__getattribute__(name)
    
    def getArray(self, name, dims=None, dtype=None):
        try:
            return self.__getattribute__(name)
        except AttributeError:
            self.__setattr__(name, np.zeros(dims, dtype=dtype))
            return self.__getattribute__(name)
    
    def getCopy(self):
        '''
        return a deep copy of the datum instance
        '''
        return deepcopy(self)
    
    def getScalar(self, name):
        return self.__getattribute__(name)
    
    def setArray(self, name, val):
        self.__setattr__(name, val)
    
    def setScalar(self, name, val):
        self.__setattr__(name, val)