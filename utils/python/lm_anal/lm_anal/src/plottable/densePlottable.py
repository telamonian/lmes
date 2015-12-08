from copy import deepcopy
from matplotlib.colors import LogNorm, Normalize
import numpy as np

from lm_anal.src.plottable.plottable import Plottable

class DensePlottable(Plottable):
    def plot(self, fig=None, ax=None, blank=False, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        fig, ax, figKwargs, axesKwargs, pltKwargs = self.plotSetup(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)

        if len(self.h_dims)==1:
            if not blank:
                self.im = self.ax.plot(self.getEdgesWithPadding()[0][:-1], self.plotData, **pltKwargs)
        elif len(self.h_dims)==2:
            X, Y = np.meshgrid(*self.getEdgesWithPadding())
            if self.scale=='log':
                pltKwargs['norm'] = LogNorm()
            else:
                pltKwargs['norm'] = Normalize()
            if not blank:
                self.im = self.ax.pcolormesh(X, Y, self.plotData, **pltKwargs)
        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs

    def plotContour(self, fig=None, ax=None, scale='log',
                    contourThreshold=10**-8, contourZeros=True, minLev=-8, maxLev=-1,
                    figKwargs=None, axesKwargs=None, pltKwargs=None):
        fig, ax, figKwargs, axesKwargs, pltKwargs = self.plotSetup(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)

        minLev = np.log10(self.plotData.min()) if minLev is None else minLev
        maxLev = np.log10(self.plotData.max()) if maxLev is None else maxLev

        centers_x,centers_y = self.getBinCenters() #[edgeArr + .5 for edgeArr in self.getEdgeArrays()]

        # contour the zero valued areas at the same level as the lowest positive values
        if contourZeros:
            hist = deepcopy(self.plotData)
            m = np.min(hist[np.nonzero(hist)])
            hist[hist < contourThreshold] = contourThreshold
        else:
            hist = self.plotData
        if self.scale=='log':
            levels = np.logspace(minLev, maxLev, maxLev - minLev + 1)
            #CL = plt.contour(centers_x, centers_y, counts, norm=LogNorm(), cmap=cm.jet)
            self.im = ax.contourf(centers_x, centers_y, hist, levels=levels, norm=LogNorm(), **pltKwargs)
        else:
            levels = np.linspace(minLev, maxLev, max([maxLev - minLev + 1, 8]))
            self.im = ax.contourf(centers_x, centers_y, hist, levels=levels, **pltKwargs)

        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs

    def plotSetup(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        fig, ax, figKwargs, axesKwargs, pltKwargs = super().plot(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)
        pad = 20

        if len(self.h_dims)==1:
            if self.scale=='log':
                self.ax.set_yscale('log')

            # self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])

            #             self.ax.set_xlabel(self.axLabels[0])
            self.ax.set_xlabel(self.getXLabel(), labelpad=pad)
            self.ax.set_ylabel(self.getYLabel(), labelpad=pad + 10)

        elif len(self.h_dims)==2:
            if 'cmap' not in pltKwargs:
                pltKwargs['cmap'] = self.cmBad.jet

            # self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
            # self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])

            self.ax.set_xlabel(self.getXLabel(), labelpad=pad)
            self.ax.set_ylabel(self.getYLabel(), labelpad=pad + 10)

        self.resizeLabels()
        self.setLims()

        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs

    def plotColorbar(self, ax=None, im=None, label='probability', **pltKwargs):
        im = im if im is not None else self.im
        if ax is None:
            self.axCBar = self.fig.add_axes([1.05, 0.12, 0.03, 0.79])   #([0.95, 0.12, 0.03, 0.79])
        else:
            self.axCBar = ax
#         t = np.logspace(-4,10,base=10,num=20)

        self.cbar = self.fig.colorbar(im, cax=self.axCBar, **pltKwargs) #, ticks=t,
        self.cbar.set_label(label, rotation=270, labelpad=75)

        self.resizeAxisLabels(ax=self.axCBar)
        self.resizeTickLabels(ax=self.axCBar)

    def setLims(self):
        if len(self.h_dims)==1:
            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])
        elif len(self.h_dims)==2:
            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
            self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])