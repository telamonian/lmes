from copy import copy as shallowCopy
from collections import OrderedDict
from pathlib import Path

from lm_anal.src.helper import LazyClass

lzTransforms = LazyClass(modName='lm_anal.src.transform', clsName='Transforms')

__all__ = ['DataMetaclass', 'Data']

class LazyMapDescriptor(object):
    '''
    lazy initializer for the map attribute that allows us to defer map's initialization until the first time it's actually used
    this descriptor "masks" itself when called, so any given Data instance can only call it once
    '''
    def __get__(self, obj, ObjType):
        obj.__dict__['map'] = OrderedDict()
        if obj.dataToTransform is not None or obj.fPath is not None:
            obj.initIO()
            if obj.readIO is not None:
                # read the data in from file
                obj.readIO.rff(container=obj, full=True)
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
                obj.intIO.wtf(container=obj)
        return obj.map

class DataMetaclass(object):
    def __new__(cls, clsname, bases, dct):
        return super(DataMetaclass, cls).__new__(cls, clsname, bases, dct)

class Data(object):
    datumType = None
    Hdf5IOType = None
    SFileType = None
    
    map = LazyMapDescriptor()
    
# initializers
    def __init__(self, protobuf=None, dataToTransform=None, dataToTransformDict=None, fPath=None, transformKwargs=None):
        self.protobuf = protobuf
        # point-of-origin, tells us from whence this data came
        self.po = None
        
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
    
    def initIO(self):
        self.intIO = self.Hdf5IOType(fPath=str(self.fPath.with_suffix('.lmint')))
        if self.intIO.has():
            self.readIO = self.intIO
            return True
        
        if self.fPath.suffix=='.lm':
            # if the fPath suffix implies that f is an hdf5 file, try reading in using the hdf5IO first
            return self.initReadIO(self.Hdf5IOType, self.SFileType)
        elif self.fPath.suffix=='.sfile':
            # if the fPath suffix implies that f is an sfile file, try reading in using the sfileIO first
            return self.initReadIO(self.SFileType, self.Hdf5IOType)
        else:
            # the current default is to try reading in from the hdf5IO first
            return self.initReadIO(self.Hdf5IOType, self.SFileType)
        
#         if self.fPath.suffix=='.lm':
#             self.readIO = self.Hdf5IOType(fPath=str(self.fPath))
#             if self.readIO.has():
#                 return True
#             else:
#             # TODO: for now, the sfile stuff is unimplemented, so leave it off
#                 # self.readIO = self.SFileType(fPath=str(self.fPath))
#                 # if self.readIO.has():
#                 #     return True
#                 # else:     
#                 self.readIO = None
#                 return False
#         elif self.fPath.suffix=='.sfile':
#         # TODO: for now, the sfile stuff is unimplemented, so leave it off
#             # self.readIO = self.SFileType(fPath=str(self.fPath))
#             # if self.readIO.has():
#             #    return True
#             # else:
#                 self.readIO = self.hdf5IO(fPath=str(self.fPath))
#                 if self.readIO.has():
#                     return True
#                 else:
#                     self.readIO = None
#                     return False
    
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
        return self.map[key]

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
        if keys==None:
            return self.map.keys().__iter__()
        else:
            for key in keys:
                yield key

    def valIter(self, keys=None):
        if keys==None:
            return self.map.values().__iter__()
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

        # symmetric difference of self keys and the keys from arg
        for oldKey in data.keys() ^ keys:
            data.pop(oldKey)

        return data

    def values(self):
        return self.map.values()

# func mapping/vectorization methods
    # returns an ordered dict with keys=self.map.keys and vals=result of func
    def mapFunc(self, func, doRaise=False, **kwargs):
        retDict = OrderedDict()
        for key,val in self:
            try:
                retDict[key] = func(val, **kwargs)
            except Exception as e:
                if doRaise:
                    raise e
                else:
                    retDict[key] = None
        return retDict