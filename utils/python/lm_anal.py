#!/usr/local/bin/python
import h5py, os, sys
from collections import OrderedDict
from os import path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.colors import LogNorm
from scipy import interpolate

from fit import Fit

class MovieDir(object):
    def __init__(self, dirName):
        self.dirName = dirName
    def __enter__(self):
        self.oldDir = os.getcwd()
        try:
            os.mkdir(self.dirName, 0755)
        except OSError:
            pass
        os.chdir(path.join(self.oldDir,self.dirName))
    def __exit__(self, type, value, traceback):
        os.chdir(self.oldDir)
        
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
        axes.plot(bins[1:] - .5, n, 'r--')  #bins is the x coord of each vertical line on the histogram. n is the height of each bin. Thus, len(bins) = len(n)+1, and so the bins[1:] weirdness
        fig.set_size_inches(36,24)
        axes.set_xticks(range(-100,101,5))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        self.Savefig(fig, '_hist')
        print 'done'
        
    def TrajMov(self, trajID=0):
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        xoparam = self.sims[trajID].oparam[0:2000]
        nAll, binsAll, patchesAll = axes.hist(xoparam, bins=200, range=(-100,100))
        axes.cla()
        
        # axes.plot(bins[1:] - .5, n, 'r--')
        #axes.plot(xnewAll, ynewAll, 'r--')
        fig.set_size_inches(9,6)
        axes.set_xticks(range(-100,101,10))
        axes.set_xlim((-80,80))
        axes.set_ylim((0,1.05*np.max(nAll)))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        axes.invert_yaxis()
        
        # yoparamAll = interpolate.splev(xoparam,tck,der=0)
        with MovieDir(self.Figname('_trajMovDiscrete',ext=False)) as dirName:
            for i,oparam in enumerate(xoparam):
                nCum, binsCum, patchesCum = axes.hist(xoparam[:i+1], bins=200, range=(-100,100))
                idx = np.argmin(np.abs(binsCum[1:] - .5 - oparam))
                #yoparamNow = interpolate.splev([oparam], tckCum, der=0)
                #dot = axes.scatter([oparam],[yoparamNow[0]],s=100,c='g')
                dot = axes.scatter([oparam],[nCum[idx]],s=100,c='g')
                fig.set_size_inches(9,6)
                axes.set_xticks(range(-100,101,10))
                axes.set_xlim((-80,80))
                axes.set_ylim((0,1.05*np.max(nAll)))
                axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
                axes.set_ylabel('count')
                axes.invert_yaxis()
                self.Savefig(fig, '_%09d' % i)
                print "saving frame _%09d" % i
                dot.remove()
                axes.cla()
        
        print 'done'
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4
    
    def TrajMovSmooth(self, trajID=0):
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        xoparam = self.sims[trajID].oparam[0:1000]
        nAll, binsAll, patchesAll = axes.hist(xoparam, bins=200, range=(-100,100))#, normed=True)
        axes.cla()
        
        
        tckAll = interpolate.splrep(binsAll[1:] - .5,nAll,k=3,s=10)
        xnewAll = np.arange(-100,100,.1)
        ynewAll = interpolate.splev(xnewAll, tckAll, der=0)
        #splineLine = map(np.array, zip(xnew,ynew))
        #spline = axes.plot(xnew, ynew, 'b')
        
        # axes.plot(bins[1:] - .5, n, 'r--')
        #axes.plot(xnewAll, ynewAll, 'r--')
        fig.set_size_inches(9,6)
        axes.set_xticks(range(-100,101,10))
        axes.set_xlim((-80,80))
        axes.set_ylim((-.05*np.max(nAll),np.max(nAll) + .05*np.max(nAll)))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        
        
        # yoparamAll = interpolate.splev(xoparam,tck,der=0)
        with MovieDir(self.Figname('_trajMov',ext=False)) as dirName:
            for i,oparam in enumerate(xoparam):
                nCum, binsCum, patchesCum = axes.hist(xoparam[:i+1], bins=200, range=(-100,100))#, normed=True)
                axes.cla()
                #idx = np.argmin(np.abs(binsCum[1:] - .5 - oparam))
                tckCum = interpolate.splrep(binsCum[1:] - .5,nCum,k=3,s=10)
                ynewCum = interpolate.splev(xnewAll, tckCum, der=0)
                ynewCumZero = [(y if y>=0 else 0) for y in ynewCum]
                axes.plot(xnewAll, ynewCumZero, 'b')
                yoparamNow = interpolate.splev([oparam], tckCum, der=0)
                yoparamNow[0] = yoparamNow[0] if yoparamNow[0] >=0 else 0
                dot = axes.scatter([oparam],[yoparamNow[0]],s=100,c='g')
                #dot = axes.scatter([oparam],[nCum[idx]],s=100,c='g')
                axes.set_xticks(range(-100,101,10))
                axes.set_xlim((-80,80))
                axes.set_ylim((-.05*np.max(nAll),np.max(nAll) + .05*np.max(nAll)))
                axes.invert_yaxis()
                self.Savefig(fig, '_%09d' % i)
                print "saving frame _%09d" % i
                dot.remove()
        print 'done'
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4
    
    
    
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

class FFluxBiphasic(Biphasic):
    pass

class FFluxBiphasics(Biphasics):
    child = FFluxBiphasic
    
    def SortByInterface(self):
        if self.oparam==None:
            self.Pdf()
        self.interfaces = {}
        for sim in self.sims:
            self.interfaces[sim.oparam[0]] = self.interfaces.get(sim.oparam[0], []) + [sim.oparam]
        self.interfacesSorted = OrderedDict(sorted(self.interfaces.items(), key=lambda x: x[0]))
        
    
    def TrajMovSmooth(self):
        if self.oparam==None:
            self.SortByInterface()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        nAll, binsAll, patchesAll = axes.hist(xoparam, bins=200, range=(-100,100))#, normed=True)
        axes.cla()
        
        
        tckAll = interpolate.splrep(binsAll[1:] - .5,nAll,k=3,s=10)
        xnewAll = np.arange(-100,100,.1)
        ynewAll = interpolate.splev(xnewAll, tckAll, der=0)
        #splineLine = map(np.array, zip(xnew,ynew))
        #spline = axes.plot(xnew, ynew, 'b')
        
        # axes.plot(bins[1:] - .5, n, 'r--')
        #axes.plot(xnewAll, ynewAll, 'r--')
        fig.set_size_inches(9,6)
        axes.set_xticks(range(-100,101,10))
        axes.set_xlim((-80,80))
        axes.set_ylim((-.05*np.max(nAll),np.max(nAll) + .05*np.max(nAll)))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        
        
        # yoparamAll = interpolate.splev(xoparam,tck,der=0)
        with MovieDir(self.Figname('_trajMov',ext=False)) as dirName:
            for i,oparam in enumerate(xoparam):
                nCum, binsCum, patchesCum = axes.hist(xoparam[:i+1], bins=200, range=(-100,100))#, normed=True)
                axes.cla()
                #idx = np.argmin(np.abs(binsCum[1:] - .5 - oparam))
                tckCum = interpolate.splrep(binsCum[1:] - .5,nCum,k=3,s=10)
                ynewCum = interpolate.splev(xnewAll, tckCum, der=0)
                ynewCumZero = [(y if y>=0 else 0) for y in ynewCum]
                axes.plot(xnewAll, ynewCumZero, 'b')
                yoparamNow = interpolate.splev([oparam], tckCum, der=0)
                yoparamNow[0] = yoparamNow[0] if yoparamNow[0] >=0 else 0
                dot = axes.scatter([oparam],[yoparamNow[0]],s=100,c='g')
                #dot = axes.scatter([oparam],[nCum[idx]],s=100,c='g')
                axes.set_xticks(range(-100,101,10))
                axes.set_xlim((-80,80))
                axes.set_ylim((-.05*np.max(nAll),np.max(nAll) + .05*np.max(nAll)))
                axes.invert_yaxis()
                self.Savefig(fig, '_%09d' % i)
                print "saving frame _%09d" % i
                dot.remove()
        print 'done'
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4    

if __name__=="__main__":
    fname = sys.argv[1]
#     ffluxbiphasics = FFluxBiphasics(fname)
#     ffluxbiphasics.SortByInterface()
#     ffluxbiphasics.TrajMovSmooth()
    biphasics = Biphasics(fname)
    if sys.argv[2]=='hist':
        biphasics.Hist()
    elif sys.argv[2]=='hist2d':
        biphasics.Hist2D()
    elif sys.argv[2]=='passage':
        biphasics.Passage()
    elif sys.argv[2]=='tcourse':
        biphasics.Tcourse()
    elif sys.argv[2]=='trajmov':
        biphasics.TrajMov()
    elif sys.argv[2]=='trajmovsmooth':
        biphasics.TrajMovSmooth()