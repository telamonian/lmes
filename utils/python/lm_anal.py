#!/usr/local/bin/python
import h5py, sys
import numpy as np
from os import path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.colors import LogNorm

from fit import Fit

class Sim(object):

    def __init__(self, simdata):
        self.times = simdata['SpeciesCountTimes']
        self.counts = simdata['SpeciesCounts']
        self.oparam = None
    
    def Rcoords(self):
        pass
    
    def Pdf(self):
        pass
    
    def Passage(self):
        if self.oparam==None:
            self.Pdf()
        dwells = []
        basin = False
        #get initial basin, then throw out all data before 1st switch
        i = 0
        while not basin:
            basin = self.Basin(i)
            i += 1
        oldbasin = basin
        while basin==oldbasin or not basin:
            basin = self.Basin(i)
            i += 1
        #set first entry time. When the distro swings to the other basin, add dwell time to list and set new entry time
        oldbasin = basin
        entryt = self.times[i-1]
        while i < len(self.oparam):
            while (basin==oldbasin or not basin) and i < len(self.oparam):
                basin = self.Basin(i)
                i += 1
            if i < len(self.oparam):
                oldbasin = basin
                if self.times[i-1] - entryt > self.times[1] - self.times[0]:
                    dwells.append(self.times[i-1] - entryt)
                    entryt = self.times[i-1]
        return dwells
    
    def Progress(self, maxt):
        return self.times[-1]/float(maxt)
        
class Sims(object):
    child = Sim

    def __init__(self, fname):
        self.fname = fname
        self.f = h5py.File(fname)
        self.params = self.f['/Parameters'].attrs
        self.maxtime = self.params['maxTime']
        self.writeinterval = self.params['writeInterval']
        self.sims = []
        for simdata in self.f['/Simulations'].values():
            self.sims.append(self.__class__.child(simdata))
        self.oparam = None
        self.rcoords = None
    
    def Figname(self, suffix, ext=True):
        if ext:
            extension = '.png'
        else:
            extension = ''
        return self.fname.split('.')[0] + suffix + extension
    
    def Savefig(self, fig, suffix):
        fname = self.Figname(suffix)
        fig.savefig(fname, bbox_inches='tight', transparent=True)
    
    def Rcoords(self):
        self.rcoords = np.array([[],[]])
        for sim in self.sims:
            sim.Rcoords()
            self.rcoords = np.hstack([self.rcoords, sim.rcoords])
    
    def Pdf(self, equilibriate=False):
        self.oparam = np.array([])
        for sim in self.sims:
            sim.Pdf()
            if equilibriate:
                self.oparam = np.hstack([self.oparam, sim.oparam[len(sim.oparam)/10:]])
            else:
                self.oparam = np.hstack([self.oparam, sim.oparam])
    
    def Tcourse(self, i=0):
        if self.sims[i].oparam==None:
            self.sims[i].Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        axes.plot(self.sims[i].times[::100], self.sims[i].oparam[::100])
        fig.set_size_inches(72,6)
        axes.set_yticks(range(-100,101,10))
        axes.set_xlabel('time (k)')
        axes.set_ylabel('$\Delta$ (copy num(b) - copy num(a))')
        self.Savefig(fig, '_tcourse')
        print 'done'
    
    def Hist(self):
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        n, bins, patches = axes.hist(self.oparam, bins=200, range=(-100,100), normed=True)
        axes.plot(bins[1:] - .5, n, 'r--')
        fig.set_size_inches(36,24)
        axes.set_xticks(range(-100,101,5))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        self.Savefig(fig, '_hist')
        print 'done'
    
    def Hist2D(self):
        if self.rcoords==None:
            self.Rcoords()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        axes.hist2d(self.rcoords[0,:], self.rcoords[1,:], range=[[0,100],[0,100]], bins=(100, 100))
        fig.set_size_inches(36,24)
        matplotlib.rcParams.update({'font.size': 42})
        axes.set_xlabel('copy num(A)')
        axes.set_ylabel('copy num(B)')
        self.Savefig(fig, '_hist2d')
        print 'done'
        
        fig = plt.figure(2)
        axes = plt.axes()
        print 'graphing logarithmic version now...'
        axes.hist2d(self.rcoords[0,:], self.rcoords[1,:], range=[[0,100],[0,100]], bins=(100, 100), norm=LogNorm())
        fig.set_size_inches(36,24)
        matplotlib.rcParams.update({'font.size': 42})
        axes.set_xlabel('copy num(A)')
        axes.set_ylabel('copy num(B)')
        self.Savefig(fig, '_hist2d_log')
        print 'done'
    
    def Passage(self):
        self.dwells = np.array([])
        for sim in self.sims:
            self.dwells = np.hstack([self.dwells, sim.Passage()])
        #fit dwell times to exponential
        def func(x, k): return 1-np.exp(-x*k())
        init = {'k':0}
        xdata = np.array(sorted(self.dwells.tolist()))
        ydata = np.array(range(1, len(self.dwells) + 1))/float(len(self.dwells))
        fit = Fit(func, init, xdata=xdata, ydata=ydata)
        print 'graphing now...'
        fit.ScatterPlot(self.Figname('_passage_cdf', ext=False), clear=True)
        fit.SmoothPlot(self.Figname('_passage_cdf', ext=False), lim=('auto','auto'), labels=('time (k)','cdf'), annotate='k: %10e' % fit.params['k'].value)
        print 'done'
        print fit
    
    def Progress(self):
        progs = []
        for sim in self.sims:
            progs.append(sim.Progress(self.maxtime))
        return np.mean(progs)
    
class Biphasic(Sim):
    basindict = {'A':0,'B':1}
    
    def Rcoords(self):
        reducer = np.array([[1,2,2,0,0,0,0],[0,0,0,1,2,2,0]]).T
        self.rcoords = np.dot(self.counts, reducer).T
        
    def Pdf(self):
        reducer = np.array([-1,-2,-2,1,2,2,0])
        self.oparam = np.dot(self.counts, reducer)
        
    def Basin(self, framei):
        if self.oparam[framei] < -25:
            return 'A'
        elif self.oparam[framei] > 25:
            return 'B'
        else:
            return False

class Biphasics(Sims):
    child = Biphasic

if __name__=="__main__":
    fname = sys.argv[1]
    biphasics = Biphasics(fname)
    if sys.argv[2]=='tcourse':
        biphasics.Tcourse()
    elif sys.argv[2]=='hist2d':
        biphasics.Hist2D()
    elif sys.argv[2]=='hist':
        biphasics.Hist()
    elif sys.argv[2]=='passage':
        biphasics.Passage()