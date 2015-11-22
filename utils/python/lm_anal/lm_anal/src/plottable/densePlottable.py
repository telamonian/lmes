from matplotlib.colors import LogNorm, Normalize
import numpy as np

from lm_anal.src.plottable.plottable import Plottable

class DensePlottable(Plottable):
    def plot(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        fig, ax, figKwargs, axesKwargs, pltKwargs = super().plot(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)
        
        if len(self.h_dims)==1:
            self.ax.plot(self.getEdgesWithPadding()[0][:-1], self.plotData, **pltKwargs)
            if scale=='log':
                self.ax.set_yscale('log')
            
            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])
            
#             self.ax.set_xlabel(self.axLabels[0])
            self.ax.set_xlabel(self.getXLabel())
            self.ax.set_ylabel(self.getYLabel())
            
        elif len(self.h_dims)==2:
            X, Y = np.meshgrid(*self.getEdgesWithPadding())
            if scale=='log':
                pltKwargs['norm'] = LogNorm()
            else:
                pltKwargs['norm'] = Normalize()
            im = self.ax.pcolormesh(X, Y, self.plotData, **pltKwargs)
            self.plotColorbar(im)
            
            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
            self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])
            
            self.ax.set_xlabel(self.getXLabel())
            self.ax.set_ylabel(self.getYLabel())

        self.resizeLabels()
        
        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs
        
    def plotColorbar(self, im):
        axcolor = self.fig.add_axes([0.95, 0.12, 0.03, 0.79])
#         t = np.logspace(-4,10,base=10,num=20)
        self.fig.colorbar(im, cax=axcolor)#, ticks=t, format='$%.2e$')