from copy import deepcopy
from matplotlib.colors import LogNorm, Normalize
import numpy as np

from lm_anal.src.plottable.plottable import Plottable

class DensePlottable(Plottable):
    def plot(self, fig=None, ax=None, scale='log', figKwargs=None, axesKwargs=None, pltKwargs=None):
        # fig, ax, figKwargs, axesKwargs, pltKwargs = super().plot(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)
        fig, ax, figKwargs, axesKwargs, pltKwargs = self.plotSetup(fig=fig, ax=ax, scale=scale, figKwargs=figKwargs, axesKwargs=axesKwargs, pltKwargs=pltKwargs)

        if len(self.h_dims)==1:
            self.im = self.ax.plot(self.getEdgesWithPadding()[0][:-1], self.plotData, **pltKwargs)
#             if self.scale=='log':
#                 self.ax.set_yscale('log')
#
#             self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])
#
# #             self.ax.set_xlabel(self.axLabels[0])
#             self.ax.set_xlabel(self.getXLabel())
#             self.ax.set_ylabel(self.getYLabel())
            
        elif len(self.h_dims)==2:
            X, Y = np.meshgrid(*self.getEdgesWithPadding())
            if self.scale=='log':
                pltKwargs['norm'] = LogNorm()
            else:
                pass
                # pltKwargs['norm'] = Normalize()
            self.im = self.ax.pcolormesh(X, Y, self.plotData, **pltKwargs)
            # self.plotColorbar(self.im)
            
            # self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEgesWithPadding()[0][-1])
            # self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])
            #
            # self.ax.set_xlabel(self.getXLabel())
            # self.ax.set_ylabel(self.getYLabel())

        # self.resizeLabels()
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
            # for x in np.nditer(self.h, op_flags=['readwrite']):
            #     if x==0:
            #         x[...] = m
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

        if len(self.h_dims)==1:
            if self.scale=='log':
                self.ax.set_yscale('log')

            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-2])

            #             self.ax.set_xlabel(self.axLabels[0])
            self.ax.set_xlabel(self.getXLabel())
            self.ax.set_ylabel(self.getYLabel())

        elif len(self.h_dims)==2:
            self.ax.set_xlim(self.getEdgesWithPadding()[0][0], self.getEdgesWithPadding()[0][-1])
            self.ax.set_ylim(self.getEdgesWithPadding()[1][0], self.getEdgesWithPadding()[1][-1])

            self.ax.set_xlabel(self.getXLabel())
            self.ax.set_ylabel(self.getYLabel())

        self.resizeLabels()

        return self.fig, self.ax, figKwargs, axesKwargs, pltKwargs

    def plotColorbar(self, im=None, label='probability', tickFormat=None):
        im = im if im is not None else self.im
        self.axcolor = self.fig.add_axes([0.95, 0.12, 0.03, 0.79])
#         t = np.logspace(-4,10,base=10,num=20)
        self.cbar = self.fig.colorbar(im, cax=self.axcolor, format=None) #, ticks=t,
        self.cbar.set_label(label, rotation=270, labelpad=75)

        self.resizeAxisLabels(ax=self.axcolor)
        self.resizeTickLabels(ax=self.axcolor)