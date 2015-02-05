# coding: utf-8
'''
Created on Oct 29, 2012

@author: tel
'''
'''
Created on Sep 12, 2011

@author: tel
'''
import numpy as np
import matplotlib.pyplot as plt
import scipy as sp
import scipy.optimize as optimize

'''
general purpose fitter/plotter, written for PCBM
'''
class Parameter:
        def __init__(self, value):
            self.value = value
    
        def set(self, value):
            self.value = value
    
        def __call__(self):
            return self.value

class Fit(object):
    '''
    takes a function with x as its first variable, and a dict with initial values for all of its other variables
    xdata can be none
    '''
    def __init__(self, func, init, ydata, xdata=None, go=True):
        self.params = {}
        for key in init.keys():
            self.params[key] = Parameter(init[key])
        def fitfunc(x): return func(x, **self.params)
        self.fitfunc = fitfunc
        self.xdata = xdata
        self.ydata = np.array(ydata)
        if go:
            self.Go()
    
    def Go(self):
        self.Fit(self.fitfunc, self.params.values(), self.ydata, self.xdata)
    
    def SmoothPlot(self, outfile, prefunc=None, lim=(None,None), xrange=None, xscale=None, annotate=None, labels=(None,None), clear=False):
        if clear:
            plt.clf()
        if xscale=='log':
            if xrange==None:
                self.xsmooth = np.logspace(np.log(self.xdata[0]), np.log(self.xdata[-1]), 10000)
            else:
                self.xsmooth = np.logspace(np.log(xrange[0]), np.log(xrange[1]), 10000)
        else:
            if xrange==None:
                self.xsmooth = np.linspace(self.xdata[0], self.xdata[-1], 10000)
            else:
                self.xsmooth = np.linspace(xrange[0], xrange[1], 10000)
        self.ysmooth = []
        if prefunc:
            for x in self.xsmooth:
                self.ysmooth.append(self.fitfunc(prefunc(x)))
        else:
            for x in self.xsmooth:
                self.ysmooth.append(self.fitfunc(x))
        plt.plot(self.xsmooth, self.ysmooth)
        self.CommonPlot(outfile=outfile, lim=lim, xscale=xscale, annotate=annotate, labels=labels)
    
    def ScatterPlot(self, outfile, lim=(None,None), xscale=None, annotate=None, labels=(None,None), clear=False):
        if clear:
            plt.clf()
        plt.scatter(self.xdata, self.ydata)
        self.CommonPlot(outfile=outfile, lim=lim, xscale=xscale, annotate=annotate, labels=labels)
    
    def CommonPlot(self, outfile, lim, xscale, annotate, labels):
        for i, (limf, data) in enumerate(zip((plt.xlim, plt.ylim),(self.xdata, self.ydata))):
            if lim[i]==None:
                pass
            elif lim[i]=='auto':
                limf((min(data),max(data)))
            else:
                limf(lim[i])
        if xscale=='log' or xscale=='symlog':
            plt.xscale(xscale)
        if annotate:
            plt.annotate(annotate, xy=(self.xdata[len(self.xdata)/2], self.ydata[len(self.ydata)/2]),  xycoords='data',
                xytext=(50, 30), textcoords='offset points',
                arrowprops=dict(arrowstyle="->"))
        if labels[0]:
            plt.xlabel(labels[0], fontsize='large')
        if labels[1]:    
            plt.ylabel(labels[1], fontsize='large')
        plt.ticklabel_format(style='sci', axis='x', scilimits=(1e5,1e10))
        plt.savefig(outfile+'.png', bbox_inches='tight', transparent=True)
    
    def SetParam(self, param, val):
        self.params[param] = Parameter(val)
    
    def Det(self):
        sstot = np.sum((self.ydata - np.mean(self.ydata))**2)
        sserr = np.sum((self.ydata - self.fitfunc(self.xdata))**2)
        return 1 - (sserr/sstot)
    
    def Chi2(self):
        return np.sum((self.ydata - self.fitfunc(self.xdata))**2)
    
    def RMSD(self):
        return np.sqrt(np.mean((self.ydata - self.fitfunc(self.xdata))**2))
    
    @staticmethod
    def Fit(function, parameters, y, x = None):
        def f(params):
            i = 0
            for p in parameters:
                p.set(params[i])
                i += 1
            return y - function(x)
    
        if x is None: x = sp.arange(y.shape[0])
        p = [param() for param in parameters]
        optimize.leastsq(f, p, maxfev = 100000)

    def __str__(self):
        s = ''
        for key, val in self.params.items():
            s += '%s:\t%.10e\n' % (key, val())
        s += 'the coefficient of determination is:\t%.10e\n' % self.Det()
        s += 'the chi**2 value is:\t%.10e\n' % self.Chi2()
        s += 'the RMSD is:\t%.10e' % self.RMSD()
        return s
    
if __name__=='__main__':
    #def func(x,m,n0,pk0): return m()*10**(-m()*n0()*(x-pk0()))/(1 + 10**(-m()*n0()*(x-pk0())))
    #init = {'m':1, 'n0':1, 'pk0':7}
    def func(x, m, b): return x*m() + b()
    init = {'m':-1,'b':1}
    x = [1.945819713, 2.032576636, 2.119333246,2.206088602,2.292845545,2.379602434,2.466358427,2.553114511,2.639870896,2.726626333]
    y = [2.367925976, 2.007955563, 1.556272365, 1.145571991, 0.71020362, 0.269130378, -0.17013317, -0.601159359, -1.03974977, -1.498368779, ]
    fit = Fit(func, init, x, y)
    print(fit)
    fit.SmoothPlot('test')
    
    