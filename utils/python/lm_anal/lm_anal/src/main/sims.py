from collections import OrderedDict
from pathlib import Path
import re

from lm_anal.src.main import Sim

class SimsMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimsMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sims(object):
    hdf5Synonyms = ['hdf5', 'lm', '.lm']
    sfileSynonyms = ['hdfs', 'sfile', '.sfile']
    
    @staticmethod
    def parseKeyFromPath(path, rootPath):
        if path==rootPath:
            relPath = Path(path.name)
        else:
            relPath = path.relative_to(rootPath)
        relPathParts = [part for part in relPath.parent.parts if part!='/']
        tokenList = [relPath.stem]
        for part in relPathParts:
            for tokens in part.split('_-_'):
                tokenList = [tuple(tokens.split('_'))] + tokenList
                
        if len(tokenList)==1:
            return tokenList[0]
        else:
            return tuple(tokenList)
    
    def __init__(self, rootPath, fType='hdf5', **kwargs):
        self.initFType(fType)
        self.map = OrderedDict()
        self.rootPath = Path(rootPath)
        self.initSims(**kwargs)
    
    def initFType(self, fType):
        '''
        initialize fType with some normalization/sanity checks
        '''
        if fType in self.hdf5Synonyms:
            self.fType = 'hdf5'
            self.suffix = '.lm'
        elif fType in self.sfileSynonyms:
            self.ftype = 'sfile'
            self.suffix = '.sfile'
        else:
            raise

    def initSim(self, key, fPath, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            self.map[key] = Sim(fPath=fPath, name=key, **kwargs)
            return self.map[key]
    
    def initSims(self, **kwargs):
        if self.rootPath.is_file():
            fPaths = (self.rootPath,)
        else:
            fPaths = self.rootPath.rglob('*{}'.format(self.suffix))
        
        for fPath in fPaths:
            key = self.parseKeyFromPath(fPath, self.rootPath)
            self.initSim(key, fPath, **kwargs)
            
        if len(self.map)==0:
            for fPath in self.rootPath.rglob('*{}'.format('.lmint')):
                key = self.parseKeyFromPath(fPath, self.rootPath)
                self.initSim(key, fPath, **kwargs)
        
    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        return self.map[key]
    
    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.__iter__()
    
    def items(self):
        return self.map.items()
    
    def keys(self):
        return self.map.keys()
    
    def values(self):
        return self.map.values()
    
#     def map(self, recipeName, **kwargs):
#         self.__getattribute__('%sMap' % recipeName)(**kwargs)
#         
#     def OParamHistsMap(self, tilingIDs, **kwargs):
#         for sim in self:
#             sim.map('OParamHists', tilingIDs=tilingIDs)