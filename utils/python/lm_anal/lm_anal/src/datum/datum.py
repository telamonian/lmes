from copy import deepcopy
import numpy as np
import re
import types

from lm_anal.src.datum.datumPropertySpec import DatumPropertySpec as DPSpec
from lm_anal.src.helper import CamelCaseUpper
from lm_anal.src.spec import DatumSpecs

DEBUG_GETTERS_SETTERS = False

__all__ = ['DatumMetaclass', 'Datum']

def DefAliasProp(name, spec, dct):
    @property
    def prop(self):
        return self.__getattribute__(spec['targetName'])
    @prop.setter
    def prop(self, val):
        self.__setattr__(spec['targetName'], val)
    dct[name] = prop

def DefNPArrayProp(name, spec, dct):
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
    dct[name] = prop

def DefNPHistogramProp(name, spec, dct):
    cache = '_%s' % name
    cache_dirty = '%s_cache_dirty' % name
    dimsName = '%s_dims' % name
    edgesName = '%s_edges' % name
    mask = '%s_mask' % name
    raw = '%s_raw' % name
    threshold = '%s_threshold' % name
    weight = '%s_weight' % name
    
    @property
    def prop(self):
        if self.__getattribute__(cache_dirty):
            self.__getattribute__(cache)[...] = self.__getattribute__(raw)*self.__getattribute__(weight)
            
            zeroMask = np.logical_or(self.__getattribute__(mask), self.__getattribute__(raw)<self.__getattribute__(threshold))
            self.__getattribute__(cache)[zeroMask] = 0
            
            self.__setattr__(cache_dirty, False)
            
        return self.__getattribute__('_'+name)
    @prop.setter
    def prop(self, val):
        if hasattr(val, 'read_direct') and callable(getattr(val, 'read_direct', None)):
            # initialize the array if it doesn't already exist
            self.getArray(dims=val.shape, dtype=spec['dtype'], name='_'+name)
            val.read_direct(self.__getattribute__('_'+name))
        else:
            self.__setattr__('_'+name, val)
    dct[name] = prop
    
    dimsSpec = DPSpec(dtype='int', name=dimsName, paths=(name,'_dims',), storageType='numpy', type='array')
    edgesSpec = DPSpec(dtype='float', name=edgesName, paths=(name,'_edges',), storageType='numpy', type='array')
    maskSpec = DPSpec(dtype='bool', name=mask, paths=(name,'_mask',), storageType='numpy', type='array')
    rawSpec = DPSpec(dtype='float', name=raw, paths=(name,'_raw',), storageType='numpy', type='array')
    
    SetPropertyBySpec(dimsName, dimsSpec, dct)
    SetPropertyBySpec(edgesName, edgesSpec, dct)
    SetPropertyBySpec(mask, maskSpec, dct)
    SetPropertyBySpec(raw, rawSpec, dct)
    
    def initializer(self, dims=None, edges=None):
        if dims is not None:
            self.__setattr__(dimsName, dims)
        if edges is not None:
            self.__setattr__(edgesName, edges)
        self.__setattr__(cache_dirty, True)
        self.__setattr__(threshold, 0)
        self.__setattr__(weight, 1)
        
        self.__setattr__(name, np.zeros(self.__getattribute__(dimsName)))
        self.__setattr__(raw, np.zeros(self.__getattribute__(dimsName)))
        self.__setattr__(mask, np.zeros(self.__getattribute__(dimsName), dtype=bool))
    
    dct['init' + CamelCaseUpper(name)] = initializer

def DefProtoArrayProp(name, spec, dct):
    getterList = ['@property',
                  'def %s(self):' % name,
                  '\treturn self.protobuf.%s' % '.'.join(spec['paths'])]
    setterList = ['@%s.setter' % name,
                  'def %s(self, val):' % name,
                  '\ttry:',
                  '\t\tself.protobuf.%s.extend(val.astype(%s).flatten().tolist())' % ('.'.join(spec['paths']), spec['dtype']),
                  '\texcept AttributeError:',
                  '\t\ttmpArr=np.zeros(val.shape, dtype=%s)' % spec['dtype'],
                  '\t\tval.read_direct(tmpArr)',
                  '\t\tself.protobuf.%s.extend(tmpArr.flatten().tolist())' % '.'.join(spec['paths'])]
    DefProtoPropFinish(name, getterList, setterList, dct)
    
def DefProtoScalarProp(name, spec, dct):
    getterList = ['@property',
                  'def %s(self):' % name,
                  '\treturn self.protobuf.%s' % '.'.join(spec['paths'])]
    setterList = ['@%s.setter' % name,
                  'def %s(self, val):' % name,
                  '\tself.protobuf.%s = %s(val)' % ('.'.join(spec['paths']), spec['dtype'])]
    DefProtoPropFinish(name, getterList, setterList, dct)

def DefProtoPropFinish(name, getterList, setterList, dct):
    if DEBUG_GETTERS_SETTERS:
        # list[-1:-1] = [otherList] inserts the elements of otherList in front of the final element of list
        indent = re.match('(\t*)', getterList[-1]).group(1)
        getterList[-1:-1] = ["%sprint('getter for the %s property was called')" % (indent, name)]
        indent = re.match('(\t*)', setterList[-1]).group(1)
        setterList[-1:-1] = ["%sprint('setter for the %s property was called')" % (indent, name)]
    execList = ['dct[name] = %s' % name]
    exec('\n'.join(getterList + setterList + execList))

def SetPropertyBySpec(name, spec, dct):
    _propertyNames = dct.get('_propertyNames', set())
    _propertyNames.add(name)
    dct['_propertyNames'] = _propertyNames
    
    if spec['type']=='alias':
        DefAliasProp(name, spec, dct)
    elif spec['type']=='array':
        if spec['storageType']=='numpy':
            DefNPArrayProp(name, spec, dct)
        elif spec['storageType']=='protobuf':
            DefProtoArrayProp(name, spec, dct)
    elif spec['type']=='embedded':
        pass
    elif spec['type']=='histogram':
        if spec['storageType']=='numpy':
            DefNPHistogramProp(name, spec, dct)
        elif spec['storageType']=='protobuf':
            raise
    elif spec['type']=='scalar':
        if spec['storageType']=='protobuf':
            DefProtoScalarProp(name, spec, dct)
    elif spec['type']=='special':
        # in this case, the property will have been defined in the normal way and the associated propertySpec is just for metadata purposes
        pass
    else:
        raise

class DatumMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        if 'propertySpecs' in dct:
            for name,spec in dct['propertySpecs'].items():
                SetPropertyBySpec(name, spec, dct)
        return super(DatumMetaclass, cls).__new__(cls, clsname, bases, dct)
    
    # TODO: reorg things so that this property is called 'propertySpecs' and the class atribute is '_propertySpecs'
    @property
    def combinedPropertySpecs(cls):
        newDatumSpecs = DatumSpecs()
        newDatumSpecs.update(*[datumType.propertySpecs for datumType in cls.__mro__ if hasattr(datumType, 'propertySpecs')])
        return newDatumSpecs
    
    @property
    def propertyNames(cls):
        return set().union(*map(lambda x: x._propertyNames if hasattr(x, '_propertyNames') else set(), cls.__mro__))
    
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
    
    def getEmbedded(self, name):
        return self.__getattribute__(name)
    
    def getScalar(self, name):
        return self.__getattribute__(name)
    
    def setArray(self, name, val):
        self.__setattr__(name, val)
    
    def setScalar(self, name, val):
        self.__setattr__(name, val)