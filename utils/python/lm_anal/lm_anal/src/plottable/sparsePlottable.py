from matplotlib.colors import LogNorm, Normalize
import numpy as np

from lm_anal.src.plottable.plottable import Plottable

class SparsePlottable(Plottable):
    def plot(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        fig, ax, figKwargs, axesKwargs, pltKwargs = super().plot(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)
        
        for dataCol in self.plotData.T:
            self.ax.plot(self.getEdgesWithPadding()[0][:-1], dataCol, **pltKwargs)
        
        if scale=='log':
            self.ax.set_yscale('log')
        
        self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])
        
        self.ax.set_xlabel(self.axLabels[0])
        self.ax.set_ylabel('counts')
            
#         elif len(self.h_dims)==2:
#             X, Y = np.meshgrid(*self.getEdgesWithPadding())
#             if scale=='log':
#                 pltKwargs['norm'] = LogNorm()
#             else:
#                 pltKwargs['norm'] = Normalize()
#             self.ax.pcolormesh(X, Y, self.plotData, **pltKwargs)
#             
#             self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
#             self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])
#             
#             self.ax.set_xlabel(self.axLabels[0])
#             self.ax.set_ylabel(self.axLabels[1])
            
        self.resizeLabels()
        
        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs