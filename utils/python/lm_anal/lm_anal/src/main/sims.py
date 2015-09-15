from copy import copy
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import re

from lm_anal.src.main import Sim
from lm_anal.src.magicDict import MagicDict

class SimsMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimsMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sims(object):
    hdf5Synonyms = {'hdf5', 'lm', '.lm'}
    lmintSynonyms = {'int', 'lmint', '.lmint'}
    sfileSynonyms = {'hdfs', 'sfile', '.sfile'}
    
    @staticmethod
    def parseKeyFromPath(path, rootPath):
        if path==rootPath:
            relPath = Path(path.name)
        else:
            relPath = path.relative_to(rootPath)
        relPathParts = [part for part in relPath.parent.parts if part!='/']
        keyElems = [('name', relPath.stem)]
        for part in relPathParts:
            for multiToken in part.split('_-_'):
                keyElems+=[tuple(multiToken.split('_'))]
                
        return tuple(keyElems)
    
    def __init__(self, rootPath, fType='hdf5', **kwargs):
        self.copying = False
        self.initFType(fType)
        self.map = MagicDict()
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
        elif fType in self.lmintSynonyms:
            self.ftype = 'lmint'
            self.suffix = '.lmint'
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
        
        self._initSims(fPaths, **kwargs)
            
        if len(self.map)==0:
            fPaths = self.rootPath.rglob('*{}'.format('.lmint'))
            self._initSims(fPaths, **kwargs)
                
    def _initSims(self, fPaths, **kwargs):
        for fPath in fPaths:
            key = self.parseKeyFromPath(fPath, self.rootPath)
            self.initSim(key, fPath, **kwargs)
            
# magic!
    def __call__(self, *args, **kwargs):
        archetype = next(self.map.values().__iter__())
        if hasattr(archetype, '__name__') and archetype.__name__[:4]=='plot':
            grid,gridLabels,singletonElems = self.getGrid()
            axArrShape = (np.product(grid.shape[1::2]), np.product(grid.shape[::2]))
            fig, axArr = plt.subplots(*axArrShape, gridspec_kw={})
            fig.set_size_inches(np.array(axArrShape)[1]*8, np.array(axArrShape)[0]*6)
            fig.tight_layout()
            for datumPlotFunc,ax,axLabel in zip(grid.ravel(), axArr.ravel(), gridLabels.ravel()):
                if datumPlotFunc is not None:
                    datumPlotFunc.__call__(fig=fig, ax=ax, *args, **kwargs)
                    
#                     fontSize = ax.get_xaxis().get_majorticklabels()[0].get_size()
#                     ax.set_title(axLabel, fontsize=fontSize)
                else:
                    ax.axis('off')
                    fig.delaxes(ax)
                    
#             plt.tight_layout()
#             return fig, axArr
        else:
            newDict = MagicDict()
            for oldKey,oldVal in self.map.items():
                if oldVal is None:
                    newDict[oldKey] = None
                else:
                    try:
                        newDict[oldKey] = oldVal.__call__(*args, **kwargs)
                    except:
                        newDict[oldKey] = None
            newSims = self.getShallowCopy()
            newSims.map = newDict
            return newSims

    def __delitem__(self, key):
        del self.map[key]
    
    def __getitem__(self, key):
        val = self.map[key]
        if isinstance(val, MagicDict):
            simsView = self.getShallowCopy()
            simsView.map = val
            return simsView
        else:
            return val
    
    def __getattr__(self, name):
        if name=='__setstate__':    # or name=='__copy__' or name=='__reduce_ex__':
            raise AttributeError
        if self.copying:
            raise AttributeError
            #return object.__getattr__(self, name)
        newDict = MagicDict()
        for oldKey,oldVal in self.map.items():
            if oldVal is None:
                newDict[oldKey] = None
            else:
                try:
                    newDict[oldKey] = oldVal.__getattribute__(name)
                except:
                    newDict[oldKey] = None
        newSims = self.getShallowCopy()
        newSims.map = newDict
        return newSims
        
    def __setitem__(self, key, val):
        self.map[key] = val

    def __iter__(self):
        return self.map.__iter__()
    
    def getGrid(self):
        return self.map.getGrid()
    
    def getShallowCopy(self):
        self.copying = True
        retCopy = copy(self)
        self.copying = False
        retCopy.copying = False
        return retCopy
    
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