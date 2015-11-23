from ast import literal_eval
from copy import copy as shallowCopy
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np
import re
from pathlib import Path
import re

from lm_anal.src.helper import HideAxesFrame, InchesToPoints, PointsToInches
from lm_anal.src.main import Sim
from lm_anal.src.magicDict import MagicDict

class SimsMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimsMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sims(object):
    privateMethodRe = re.compile(r'_[^_]*')

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
    def _call(self, *args, **kwargs):
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

    def __call__(self, *args, **kwargs):
        archetype = next(self.map.values().__iter__())
        if hasattr(archetype, '__name__') and archetype.__name__[:4]=='plot':
            self._plotGrid(self, *args, **kwargs)
        else:
            return self._call(*args, **kwargs)

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
        # this gets called if an attr isn't found in this Sims object
        if name=='__setstate__':    # or name=='__copy__' or name=='__reduce_ex__':
            # this is __getattr__ speak for 'you're out of luck, buddy'
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

    def _addGridGlobalLabels(self, fig, xLabel=None, yLabel=None):
        fontSizeX,fontSizeY = self._getFontSizesFromFigSize(fig)

        ax = fig.add_subplot(111, zorder=-1000)
        HideAxesFrame(ax)

        ax.set_xlabel(xLabel, size=fontSizeX)
        ax.set_ylabel(yLabel, labelpad=5, size=fontSizeY)

        # xPad = self._getFigPadFracFromFontSize(dim=0, fig=fig, fontSize=fontSizeX)
        # yPad = self._getFigPadFracFromFontSize(dim=1, fig=fig, fontSize=fontSizeY)

        # plt.annotate(xLabel, xy=(0.5, 0), xytext=(0, 0),
        #                     xycoords='figure fraction', textcoords='offset points',
        #                     size=fontSizeX, ha='center', va='top')

        # xKwargs = {'x':0.5, 'y':-xPad, 'text':xLabel, 'ha':'center', 'size':fontSizeX, 's':None}
        # yKwargs = {'x':-yPad, 'y':0.5, 'text':yLabel, 'va':'center', 'rotation':'vertical', 'size':fontSizeY, 's':None}
        # for kwargs in [kwargs for kwargs in [xKwargs, yKwargs] if kwargs['text'] is not None]:
        #     fig.text(**kwargs)
        return (fontSizeX, fontSizeY)

    def _addGridColumnLabels(self, axArr, grid):
        # if there's only one column, skip this
        if grid.shape[0] < 2:
            return

        pad = 5 # in points
        columnLabels = self._genGridColumnLabels(grid)

        for ax,label in zip(axArr[0,:].ravel(), columnLabels):
            ax.annotate(label, xy=(0.5, 1), xytext=(0, pad),
                        xycoords='axes fraction', textcoords='offset points',
                        size='large', ha='center', va='baseline')

    def _addGridRowLabels(self, axArr, grid):
        # if there's only one row, skip this
        if grid.shape[1] < 2:
            return

        pad = 5 # in points
        rowLabels = self._genGridRowLabels(grid)

        for ax,label in zip(axArr[:,-1].ravel(), rowLabels):
            ax.annotate(label, xy=(1, 0.5), xytext=(pad, 0),
                        xycoords='axes fraction', textcoords='offset points',
                        size='large', ha='left', va='center', rotation='vertical')

    def _formatGridPlot(self, fig, axArr, grid):
        # the grid cotains bound functions instead of datums, so we need the below ugliness to get at datum.getXLabel()
        figXLabel = grid['obj'].ravel()[0].__self__.getXLabel()        #axArr.ravel()[0].get_xlabel()
        figYLabel = grid['obj'].ravel()[0].__self__.getYLabel()        #axArr.ravel()[0].get_ylabel()
        for ax in axArr.ravel():
            # once for x...
            ax.set_xlabel('')
            # this is going to need to be more complex to do the tick pruning I wanted...
            # for tickText in ax.get_xaxis().get_majorticklabels()[0:1] + ax.get_xaxis().get_majorticklabels()[-1:]:
            #     print(tickText.get_visible())
            #     tickText.set_visible(False)

            # and once for y
            ax.set_ylabel('')

            # for now, set every axes to have an equal aspect ratio. may want to add way to turn this on/off at the datum level
            ax.set_aspect('equal')

        self._addGridRowLabels(axArr, grid)
        self._addGridColumnLabels(axArr, grid)

        self._addGridGlobalLabels(fig, figXLabel, figYLabel)
        fig.tight_layout()

    def _genGridColumnLabels(self, grid):
        return self._genGridLineLabels(grid, dim=0)

    def _genGridRowLabels(self, grid):
        return self._genGridLineLabels(grid, dim=1)

    def _genGridLineLabels(self, grid, dim):
        labels = []
        lineSlice = (0,)*dim + (slice(None),) + (0,)*(len(grid['label'].shape) - dim - 1)
        for i,gridLabel in enumerate(grid['label'][lineSlice].ravel()):
            # difference out the label that changes along this dim
            otherSet = set([tup for tups in grid['label'][lineSlice][:i] for tup in tups] +
                           [tup for tups in grid['label'][lineSlice][i+1:] for tup in tups])
            # this set should contain only a single tuple
            uniqueLabelSet = set(gridLabel) - otherSet
            # stupid iterator crap to get the 'first' element in a set
            labels.append(next(uniqueLabelSet.__iter__()))
        return labels

    def _getFigPadFracFromFontSize(self, dim, fig, fontSize):
        extent = fig.get_size_inches()[dim]
        return PointsToInches(fontSize)/extent

    def _getFontSizesFromFigSize(self, fig, scaleFactor=.07):
        fontSizes = []
        extent = min(fig.get_size_inches())
        for i in range(len(fig.get_size_inches())):
            fontSizeInInches = extent*float(scaleFactor)
            fontSizes.append(InchesToPoints(fontSizeInInches))
        return fontSizes

    def _plotGrid(self, *args, **kwargs):
        # self ends up at the front of args (due to the nature of method signatures), so pop it off
        args = list(args); args.pop(0)
        grid,singletonElems = self.getGrid()

        # unroll higher-D grids into 2D grids
        axArrShape = (np.product(grid.shape[1::2], dtype=int), np.product(grid.shape[::2], dtype=int))
        fig, axArr = plt.subplots(*axArrShape, gridspec_kw={}, sharex=True, sharey=True)
        # .subplots() flattens away dimensions of length 1, but we want a 2D axArr so add them back in if necessary
        if not isinstance(axArr, np.ndarray):
            axArr = np.array([axArr], dtype='O').reshape(1,1)
        elif len(axArr.shape) < 2:
            axArr = axArr.reshape(1,-1)
        fig.set_size_inches(np.array(axArrShape)[1]*8, np.array(axArrShape)[0]*8)
        fig.tight_layout()
        # delay turning any axes off until after formatting to preserve correct placement/spacing
        badAxes = []
        for i,(datumPlotFunc,ax) in enumerate(zip(grid['obj'].ravel(), axArr.T.ravel())):
            if datumPlotFunc is not None:
                datumPlotFunc.__call__(fig=fig, ax=ax, *args, **kwargs)
            else:
                badAxes.append(i)
                # fig.delaxes(ax)
                # ax.set_frame_on(False)
        self._formatGridPlot(fig, axArr, grid)
        for ax in (axArr.ravel()[i] for i in badAxes):
            HideAxesFrame(ax)

    def getGrid(self):
        return self.map.getGrid()
    
    def getShallowCopy(self):
        self.copying = True
        retCopy = shallowCopy(self)
        self.copying = False
        retCopy.copying = False
        return retCopy

# explicit functional methods for dealing with objects in the underlying MagicDict
    def call(self, *args, **kwargs):
        return self._call(*args, **kwargs)

    def get(self, name):
        return self.__getattr__(name)

    def getItem(self, name):
        return self.get('__getitem__')(name)

# explicit functional methods for "collapsing" the Sims object and returning simple iterators/lists, or in some cases values
    def getItems(self):
        return list(self.items())

    def getKeys(self):
        return list(self.keys())

    def getValues(self):
        return list(self.values())

    def items(self):
        return self.map.items()

    def keys(self):
        return self.map.keys()

    def peek(self):
        return self.map.peek()

    def values(self):
        return self.map.values()

#     def map(self, recipeName, **kwargs):
#         self.__getattribute__('%sMap' % recipeName)(**kwargs)
#         
#     def OParamHistsMap(self, tilingIDs, **kwargs):
#         for sim in self:
#             sim.map('OParamHists', tilingIDs=tilingIDs)