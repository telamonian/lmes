from copy import deepcopy
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from lm_anal.src.helper import POINTS_PER_INCH

class Plottable(object):
    # TODO: doNotCopyAttrs should be subject to cooperative multiple inheritance. Can this be implmented in a metaclass?
    # these attrs should not be copied when making a deepcopy of this object
    doNotCopyAttrs = ['ax', 'axcolor', 'cbar', 'fig', 'im'] + ['file']
    initialized = False

    # make a set of color maps with reasonable 'bad' values
    Dummy = type('Dummy', (object,), {})
    cmBad = Dummy()
    for name,attr in matplotlib.cm.__dict__.items():
        if isinstance(attr, matplotlib.colors.LinearSegmentedColormap):
            cmBad.__setattr__(name, deepcopy(attr))
            cmBad.__getattribute__(name).set_bad(attr(0))
    
    # to deal with the fact that matplotlib objects don't play well with deepcopy
    def __deepcopy__(self, memo):
        cls = self.__class__
        result = cls.__new__(cls)
        memo[id(self)] = result
        for k, v in self.__dict__.items():
            if k in self.doNotCopyAttrs:
                result.__setattr__(k, None)
            else:
                try:
                    setattr(result, k, deepcopy(v, memo))
                except TypeError:
                    pass
        return result
    
    def initPlottingEnvironment(self):
        if not Plottable.initialized:
            matplotlib.rcParams.update({'font.size': 20, 'axes.formatter.limits':(-4,4)})

            Plottable.initialized = True
    
    def plot(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        # init attrs directly from args
        self.scale = scale

        self.initPlottingEnvironment()
        
        if axesKwargs is None:
            axesKwargs = {}
        if figKwargs is None:
            figKwargs = {'figsize':(12,12)}
        if pltKwargs is None:
            pltKwargs = {}
        
        if fig is None:
            self.fig = plt.figure(**figKwargs)
        else:
            self.fig = fig
        if ax is None:
            self.ax = self.fig.gca(**axesKwargs)
        else:
            self.ax = ax
        
        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs
    
    def getAxesSize(self):
        bbox = self.ax.get_window_extent().transformed(self.fig.dpi_scale_trans.inverted())
        return bbox.width, bbox.height
    
    def getFontSizesFromAxesSize(self, scaleFactor=.07):
        fontSizes = []
        for length in self.getAxesSize():
            fontSizeInInches = length*float(scaleFactor)
            fontSizes.append(fontSizeInInches*POINTS_PER_INCH)
        return fontSizes

    def getXLabel(self):
        return ''

    def getYLabel(self):
        return ''

    def getAxLabels(self):
        return (self.getXLabel(), self.getYLabel())

    def resizeLabels(self, fontSize=None):
        self.resizeAxisLabels(fontSize)
        self.resizeTickLabels(fontSize)
    
    def resizeAxisLabels(self, ax=None, fontSize=None):
        ax = self.ax if ax is None else ax
        if fontSize==None:
            fontSizes = self.getFontSizesFromAxesSize()
        else:
            fontSizes = [fontSize]*2

        for i,axis in enumerate((ax.get_xaxis(), ax.get_yaxis())):
            axis.get_label().set_size(fontSizes[i])
    
    def resizeTickLabels(self, ax=None, fontSize=None):
        ax = self.ax if ax is None else ax
        if fontSize==None:
            fontSize = np.min(self.getFontSizesFromAxesSize())
        
        for axis in (ax.get_xaxis(), ax.get_yaxis()):
            for tickLabel in axis.get_majorticklabels():
                tickLabel.set_size(fontSize)

    def savefig(self, *args, **kwargs):
        if len(args)==0:
            ext = '.eps' if 'ext' not in kwargs else kwargs.pop('ext')
            args = [str(self.fPath.with_suffix(ext))]
        if 'bbox_inches' not in kwargs:
            kwargs['bbox_inches'] = 'tight'
        plt.savefig(*args, **kwargs)