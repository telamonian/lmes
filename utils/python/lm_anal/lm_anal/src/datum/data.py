from copy import copy as shallowCopy
from collections import OrderedDict
from pathlib import Path

import lm_anal.src.helper as hlp

lzTransforms = hlp.LazyClass(modName='lm_anal.src.transform', clsName='Transforms')

__all__ = ['DataMetaclass', 'Data']

class LazyMapDescriptor(object):
    '''
    lazy initializer for the map attribute that allows us to defer map's initialization until the first time it's actually used
    this descriptor "masks" itself when called, so any given Data instance can only call it once
    '''
    def __get__(self, obj, ObjType):
        obj.__setattr__('map', OrderedDict())
        if obj.dataToTransform is not None or obj.fPath is not None:
            obj.initIO()
            if obj.readIO is not None:
                # read the data in from file
                obj.readIO.rff(container=obj, excludedFields=obj.excludedFieldsRead)
                # record the object's point-of-origin
                if obj.readIO==obj.intIO:
                    obj.po = '.lmint'
                else:
                    obj.po = '.lm'
            else:
                # produce the data via transform
                transformKwargs = obj.dataToTransformDict.copy()
                transformKwargs.update(obj.transformKwargs)
                lzTransforms(srcs=obj.dataToTransform, dsts=obj, **transformKwargs)
                # record the object's point-of-origin as a transform
                obj.po = 'transform'
                # save the data to the .lmint file
                obj.intIO.wtf(container=obj, excludedFields=obj.excludedFieldsWrite)
        return obj.map

class DataMetaclass(object):
    def __new__(cls, clsname, bases, dct):
        return super(DataMetaclass, cls).__new__(cls, clsname, bases, dct)

class Data(object):
    datumType = None
    hdf5IOType = None
    sfileType = None
    
    map = LazyMapDescriptor()
    
# initializers
    def __init__(self, protobuf=None, dataToTransform=None, dataToTransformDict=None, fPath=None, lazyLoad=True, transformKwargs=None):
        '''
        calls the actual ._init() method. This makes it possible for subclasses to override ._init() while still inheriting the default parameter values in the .__init__() signature
        '''
        self._init(protobuf=protobuf, dataToTransform=dataToTransform, dataToTransformDict=dataToTransformDict, fPath=fPath, lazyLoad=lazyLoad, transformKwargs=transformKwargs)
        
    def _init(self, protobuf, dataToTransform, dataToTransformDict, fPath, lazyLoad, transformKwargs):
        self.protobuf = protobuf
        # point-of-origin, tells us from whence this data came
        self.po = None

        # sets of fields excluded from being read into and/or written out from this Data's Datums
        self.excludedFieldsRead = set()
        self.excludedFieldsWrite = set()

        self.dataToTransform = dataToTransform
        if dataToTransformDict==None:
            self.dataToTransformDict = {}
        else:
            self.dataToTransformDict = dataToTransformDict
        if fPath is not None:
            self.fPath = Path(fPath)
        else:
            self.fPath = None
        if transformKwargs is not None:
            self.transformKwargs = transformKwargs
        else:
            self.transformKwargs = {}

#         if self.dataToTransform is None and self.fPath is None:
#             self.map = {}

        if not lazyLoad:
            # .map starts out as a descriptor for lazy loading purposes, this bypass that contraption and eagerly generate map
            self.map

    def initIO(self, fPath=None):
        if fPath is not None:
            self.fPath = fPath
        self.initIntIO()
        if self.intIO.has():
            self.readIO = self.intIO
            return True

        if self.fPath.suffix=='.lm':
            # if the fPath suffix implies that f is an hdf5 file, try reading in using the hdf5IO first
            retVal = self.initReadIO(self.hdf5IOType, self.sfileType)
            return retVal
        elif self.fPath.suffix=='.sfile':
            # if the fPath suffix implies that f is an sfile file, try reading in using the sfileIO first
            return self.initReadIO(self.sfileType, self.hdf5IOType)
        else:
            # the current default is to try reading in from the hdf5IO first
            return self.initReadIO(self.hdf5IOType, self.sfileType)
        
#         if self.fPath.suffix=='.lm':
#             self.readIO = self.hdf5IOType(fPath=str(self.fPath))
#             if self.readIO.has():
#                 return True
#             else:
#             # TODO: for now, the sfile stuff is unimplemented, so leave it off
#                 # self.readIO = self.sfileType(fPath=str(self.fPath))
#                 # if self.readIO.has():
#                 #     return True
#                 # else:     
#                 self.readIO = None
#                 return False
#         elif self.fPath.suffix=='.sfile':
#         # TODO: for now, the sfile stuff is unimplemented, so leave it off
#             # self.readIO = self.sfileType(fPath=str(self.fPath))
#             # if self.readIO.has():
#             #    return True
#             # else:
#                 self.readIO = self.hdf5IO(fPath=str(self.fPath))
#                 if self.readIO.has():
#                     return True
#                 else:
#                     self.readIO = None
#                     return False

    def initIntIO(self, fPath=None):
        fPath = self.fPath if fPath is None else fPath
        self.intIO = self.hdf5IOType(fPath=str(fPath.with_suffix('.lmint')))

    def initReadIO(self, *ioTypes):
        for ioType in ioTypes:
            if ioType is None:
                continue
            self.readIO = ioType(fPath=str(self.fPath.with_suffix(ioType.suffix)))
            if self.readIO.has():
                return True
        self.readIO = None
        return False
        
    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = self.datumType(**kwargs)
            return self.map[key]
    
# magic methods and the like
    def __call__(self, key):
        try:
            return self.map[key]
        except KeyError as e:
            # key is a tuple-of-tuples
            keySet = {key}
            for datumKey,val in self.map.items():
                if keySet <= hlp.Setify(datumKey):
                    return val
            # key is a tuple-of-tulpes-of-tuples
            keySet = hlp.Setify(key)
            for datumKey,val in self.map.items():
                if keySet <= hlp.Setify(datumKey):
                    return val
            raise e

    def __contains__(self, key):
        return key in self.map

    def __delitem__(self, key):
        del self.map[key]

    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.items().__iter__()

    def keyIter(self, keys=None):
        # the keys keyword is here mostly for symmetry with valIter
        if keys is None:
            yield from self.keys().__iter__()
        else:
            for key in keys:
                yield key

    def valIter(self, keys=None):
        if keys is None:
            yield from self.values().__iter__()
        else:
            for key in keys:
                yield self[key]

# accessors
    def keys(self):
        return self.map.keys()
    
    def items(self):
        return self.map.items()
    
    def peek(self):
        '''
        return the "first" entry from self.map
        '''
        return next(self.map.values().__iter__())

    def sliceByKeys(self, keys, inPlace=False):
        if inPlace:
            data = self
        else:
            data = shallowCopy(self)
            data.map = shallowCopy(self.map)

        # symmetric difference of self keys and the keys from arg
        for oldKey in set(data.keys()) ^ hlp.Setify(keys):
            data.pop(oldKey)

        return data

    def values(self):
        return self.map.values()

# io
    def dfint(self, fPath=None, raiseIfNotExists=False):
        '''
        delete from int
        '''
        if fPath is not None:
            self.initIntIO(fPath=Path(fPath))
        self.intIO.dff(raiseIfNotExists=raiseIfNotExists, excludedFields=self.excludedFieldsWrite)

    def wtint(self, fPath=None, deleteIfExists=True):
        '''
        write to int
        '''
        if fPath is not None:
            self.initIntIO(fPath=Path(fPath))
        if deleteIfExists:
            self.dfint()
        self.intIO.wtf(container=self, excludedFields=self.excludedFieldsWrite)

# mutators
    def clearExcludedFields(self, *fieldNames, read=True, write=True):
        '''
        completely clears the .excludedFields... attrs if *fieldNames is empty
        otherwise, it just removes the names in *fieldNames from the .excludedFields... attrs
        '''
        if not fieldNames:
            if read:
                self.excludedFieldsRead = set()
            if write:
                self.excludedFieldsWrite = set()
        else:
            for fieldName in fieldNames:
                if read:
                    self.excludedFieldsRead.pop(fieldName)
                if write:
                    self.excludedFieldsWrite.pop(fieldName)

    def pop(self, key):
        return self.map.pop(key)

    def regen(self, fPath=None, overwriteInt=True):
        if fPath is not None:
            self.initIntIO(fPath=Path(fPath))

        if self.po is None:
            # make sure the normal IO machinery is initialized
            self.resetMap()

        if overwriteInt:
            self.dfint()

        self.resetMap()

        return self

    def resetMap(self):
        # reset the .map lazy loader
        try:
            del self.map
        except AttributeError:
            pass
        # poke the .map lazy loader
        self.map

    def setExcludedFields(self, *fieldNames, read=True, write=True):
        '''
        any fields in *fieldNames will not be read into and/or written out from this Data's Datums
        '''
        for fieldName in fieldNames:
            if read:
                self.excludedFieldsRead.add(fieldName)
            if write:
                self.excludedFieldsWrite.add(fieldName)

    def transform(self, *dataToTransform, **transformKwargs):
        lzTransforms(srcs=dataToTransform, dsts=self, **transformKwargs)

# func mapping/vectorization methods
    def mapFunc(self, func, *args, doRaise=False, **kwargs):
        # returns an ordered dict with keys=self.map.keys and vals=result of func
        retDict = OrderedDict()
        for key,datum in self:
            try:
                retDict[key] = func(datum, *args, **kwargs)
            except Exception as e:
                if doRaise:
                    raise e
                else:
                    retDict[key] = None
        return retDict

    def mapGet(self, attrName, doRaise=False):
        return self.mapMethod('__getattribute__', attrName, doRaise=doRaise)

    def mapMethod(self, methodName, *args, doRaise=False, **kwargs):
        # returns an ordered dict with keys=self.map.keys and vals=self.__getattribute__(methodName)(**kwargs)
        retDict = OrderedDict()
        for key,datum in self:
            try:
                retDict[key] = datum.__getattribute__(methodName)(*args, **kwargs)
            except Exception as e:
                if doRaise:
                    raise e
                else:
                    retDict[key] = None
        return retDict