import matplotlib
import matplotlib.pyplot as plt
import numpy as np

class Plottable(object):
    initialized = False
    
    def initPlottingEnvironment(self):
        if not Plottable.initialized:
            matplotlib.rcParams.update({'font.size': 20, 'axes.formatter.limits':(-4,4)})
            Plottable.initialized = True
    
    def plot(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
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
    
    def getAxisLabelFontSizesFromFigSize(self, scaleFactor=2):
        fontSizes = []
        for length in self.fig.get_size_inches():
            fontSize = int(length/(1/float(scaleFactor)))
            fontSizes.append(fontSize)
        return fontSizes
    
    def getTickFontSizeFromFigSize(self, scaleFactor=2):
        return int(np.min(self.fig.get_size_inches())/(1/float(scaleFactor)))
    
    def resizeLabels(self, fontSize=None):
        self.resizeAxisLabels(fontSize)
        self.resizeTickLabels(fontSize)
    
    def resizeAxisLabels(self, fontSize=None):
        if fontSize==None:
            fontSizes = self.getAxisLabelFontSizesFromFigSize()
        else:
            fontSizes = [fontSize]*2
        
        for i,axis in enumerate((self.ax.get_xaxis(), self.ax.get_yaxis())):
            axis.get_label().set_size(fontSizes[i])
    
    def resizeTickLabels(self, fontSize=None):
        if fontSize==None:
            fontSize = self.getTickFontSizeFromFigSize()
        
        print(fontSize)
        for axis in (self.ax.get_xaxis(), self.ax.get_yaxis()):
            for tickLabel in axis.get_majorticklabels():
                tickLabel.set_size(fontSize)