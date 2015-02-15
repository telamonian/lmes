#!/usr/bin/env python
import bisect as bi
from collections import OrderedDict
import h5py
from operator import attrgetter
from os import path
from mpl_toolkits.mplot3d import Axes3D
import matplotlib
#matplotlib.use('Agg')
from matplotlib import animation
from matplotlib.collections import PatchCollection, LineCollection
import matplotlib.cm as cm
from matplotlib.colors import LogNorm, Normalize
from matplotlib.patches import Circle, Wedge, Polygon
import matplotlib.pyplot as plt
from matplotlib.ticker import LogFormatter
import numpy as np
import os
import pickle
import pylab
import re
from scipy import interpolate
import sys

from fit import Fit

from lifelines import CoxPHFitter,KaplanMeierFitter
import pandas as pd
#from builtins import True
#matplotlib.rcParams['savefig.transparent'] = True

#booooga boooga booga

cm.hot.set_bad(cm.hot(0))
# stuff to make the perceptual color maps
pCMap = {}
rPCMap = {}
walk = next(os.walk(os.path.join(os.path.dirname(os.path.realpath(__file__)),'lm_anal','plot','colormaps')))
for fname in (fname for fname in walk[2] if fname[0]!='.'):
    with open(os.path.join(walk[0],fname)) as f:
        cVals = np.array([list(map(float,line.strip().split(','))) for line in f])
        # Setting up columns for tuples
        b3 = cVals[:,2]
        b2 = cVals[:,2]
        b1 = np.linspace(0, 1, len(b2))
        g3 = cVals[:,1]
        g2 = cVals[:,1]
        g1 = np.linspace(0,1,len(g2))
        r3 = cVals[:,0]
        r2 = cVals[:,0]
        r1 = np.linspace(0,1,len(r2))
        # Creating tuples
        R = zip(r1,r2,r3)
        G = zip(g1,g2,g3)
        B = zip(b1,b2,b3)
        # Transposing
        RGB = list(zip(R,G,B))
        rgb = list(zip(*RGB))
        # Creating color maps
        k = ['red', 'green', 'blue']
        pCMap[fname.split('.')[0]] = matplotlib.colors.LinearSegmentedColormap(fname.split('.')[0], dict(zip(k,rgb)))
        pCMap[fname.split('.')[0]].set_bad(cVals[0,:]) # set RGB value for bad pixels (like the zeros in log-norm hist2d plots)
        # Creating reverse color maps
        rgb.reverse()
        rPCMap[fname.split('.')[0]] = matplotlib.colors.LinearSegmentedColormap(fname.split('.')[0], dict(zip(k,rgb)))
        rPCMap[fname.split('.')[0]].set_bad(cVals[0,:]) # set RGB value for bad pixels (like the zeros in log-norm hist2d plots)

# Data manipulation:

def make_segments(x, y):
    '''
    Create list of line segments from x and y coordinates, in the correct format for LineCollection:
    an array of the form   numlines x (points per line) x 2 (x and y) array
    '''

    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)
    
    return segments


# Interface to LineCollection:

def colorline(x, y, z=None, cmap=pCMap['cube1'], norm=plt.Normalize(0.0, 1.0), linewidth=3, alpha=1.0, ax=None, zdir=None, zs=0):
    '''
    Plot a colored line with coordinates x and y
    Optionally specify colors in the array z
    Optionally specify a colormap, a norm function and a line width
    '''
    
    # Default colors equally spaced on [0,1]:
    if z is None:
        z = np.linspace(0.0, 1.0, len(x))
           
    # Special case if a single number:
    if not hasattr(z, "__iter__"):  # to check for numerical input -- this is a hack
        z = np.array([z])
        
    z = np.asarray(z)
    
    segments = make_segments(x, y)
    lc = LineCollection(segments, array=z, cmap=cmap, norm=norm, linewidth=linewidth, alpha=alpha)
    
    if ax is None:
        ax = plt.gca()
    
    if zdir is None:
        ax.add_collection(lc)
    else:
        ax.add_collection3d(lc, zdir=zdir, zs=zs)
    
    return lc
        
    
def clear_frame(ax=None): 
    # Taken from a post by Tony S Yu
    if ax is None: 
        ax = plt.gca() 
    ax.xaxis.set_visible(False) 
    ax.yaxis.set_visible(False) 
    for spine in ax.spines.values(): 
        spine.set_visible(False) 

def chunks(l, n):
    if n < 1:
        n = 1
    return [l[i:i + n] for i in range(0, len(l), n)]

def make_segments(x, y):
    '''
    Create list of line segments from x and y coordinates, in the correct format for LineCollection:
    an array of the form   numlines x (points per line) x 2 (x and y) array
    '''

    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)
    
    return segments

# determine whether font color of an annotation should be black or white depending on background
def AnnotationColor(r, g, b, a=1.0):
    # Counting the perceptive luminance - human eye favors green color... 
    darknessCoefficient = 1 - ( 0.299 * r + 0.587 * g + 0.114 * b);

    if (darknessCoefficient < 0.5):
       d = 0 # bright background color, so use black font
    else:
       d = 1 # dark background color, so use white font

    return (d, d, d, a)

class MovieDir(object):
    def __init__(self, dirName):
        self.dirName = dirName
    def __enter__(self):
        self.oldDir = os.getcwd()
        try:
            os.mkdir(self.dirName, 0o755)
        except OSError:
            pass
        os.chdir(path.join(self.oldDir,self.dirName))
    def __exit__(self, type, value, traceback):
        os.chdir(self.oldDir)
        
class Sim(object):
    def __init__(self, simdata, **kwargs):
        self.times = simdata['SpeciesCountTimes']
        self.counts = simdata['SpeciesCounts']
        self.oparam = None
        self.rcoords = None
        self.sweepParams = kwargs
        
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
        while (basin==oldbasin or not basin) and i < len(self.oparam):
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
    
    def Pdf(self):
        pass
    
    def Progress(self, maxt):
        return self.times[-1]/float(maxt)
    
    def Rcoords(self):
        pass

class Sims(object):
    child = Sim

    def __init__(self, fPath, type='replicateProbability', lmintOnly=False, **kwargs):
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.modTimePath = os.path.join(self.fDir, '.' + self.fName) + '.mod'
        self.intermediatePath = os.path.join(self.fDir, self.fName) + '.lmint'
        self.sweepParams = kwargs
        self.type = type
        self.oparam = None
        self.rcoords = None
        
        if type=='replicateProbability':
            if lmintOnly:
                # we only have a .lmint file and no base .lm file
                self._Load()
            elif not self.Load():
                self.Init()
        else:
            self.f = h5py.File(self.fPath)
            self.params = self.f['/Parameters'].attrs
            self.maxtime = self.params['maxTime']
            self.writeinterval = self.params['writeInterval']
            self.replicateKeys = list(self.f['/Simulations'].keys())
            self.replicateCount = len(self.replicateKeys)
            
            self.tileEdges = self.f['/Tilings/0000000/Edges']
            self.normalizedProbabilityI = self.f['/Tilings/0000000/FFluxOutput/NormalizedProbabilityI/TileVals']
            
        # logic of the following conditional:
        # if you want to unpickle an intermediate AND the raw data HAS NOT changed, then do so
        # otherwise if you want to unpickle an intermediate AND the raw data HAS changed, then work with the raw data
        # otherwise if you don't care about pickled anything, then work with the raw data
#         if unpickle==True:
#             try:
#                 if self.CheckMod():
#                     self.Load()
#                 else:
#                     self.Init()
#             except IOError:
#                 self.Init()
#         else:
#             self.Init()
    
    def __str__(self):
        outString = ''
        for key,val in self.sweepParams.items():
            outString+='%s: %s, ' % (key,val)
        return outString[:-2]
    
    def Init(self):
        self.f = h5py.File(self.fPath)
        self.params = self.f['/Parameters'].attrs
        self.maxtime = self.params['maxTime']
        self.writeinterval = self.params['writeInterval']
        self.replicateKeys = list(self.f['/Simulations'].keys())
        self.replicateCount = len(self.replicateKeys)
        self.f.close()
        
        self.histogram = []
        self.xBinCoordinates = []
        self.yBinCoordinates = []
#         self.p = []
        for key in self.replicateKeys:
            print('starting intermediate processing of replicate %07d' % int(key))
            self.f = h5py.File(self.fPath)
            simdata = self.f['/Simulations'][key]
            sim = self.__class__.child(simdata, **self.sweepParams)
            self.InitFPT()
            self.InitOParamHist
            self.
            elif self.type=='replicateProbability':
                self.RcoordsHist(sim)
                self.PdfHist()
            elif self.type=='fflux':
                self.GetFFluxOutput()
            del sim
            self.f.close()
        self.Save()
    
    def CheckMod(self):
        '''
        check the mod time file (written with SaveMod()) corresponding to this simulation file. 
        If the simulation file has changed since the mod time file was written, return False. Otherwise, return True
        '''
        try:
            with open(self.modTimePath, 'r') as modF:
                if float(modF.readline())==os.path.getmtime(self.fPath):
                    print('up to date mod file found'); return True
                else:
                    print('mod file out of date'); return False
        except FileNotFoundError:
            print('mod file does not exist'); return False
    
    def SaveMod(self):
        '''
        write out a mod time file corresponding to this simulation file.
        the format will be a single line with the time the simulation file was last modified
        to remove all .mod files from a dir tree in bash, use: rm */.[^.]*.mod
        '''
        with open(self.modTimePath, 'w') as modF:
            modF.write('%f' % os.path.getmtime(self.fPath))
    
    def _Load(self):
        with h5py.File(self.intermediatePath, 'r') as interF:
            try:
                type = interF['/Parameters'].attrs['type']
            except KeyError:
                type = 'replicateProbability'
            if type=='replicateFPT':
                pass
            elif type=='replicateProbability':
                self.histogram = []
                self.xBinCoordinates = []
                self.yBinCoordinates = []
                for key,val in interF["/Simulations/"].items():
                    self.histogram = np.zeros(val['Histogram'].shape)
                    val['Histogram'].read_direct(self.histogram)
                    self.xBinCoordinates = np.zeros(val['XBinCoordinates'].shape)
                    val['XBinCoordinates'].read_direct(self.xBinCoordinates)
                    self.yBinCoordinates = np.zeros(val['YBinCoordinates'].shape)
                    val['YBinCoordinates'].read_direct(self.yBinCoordinates)
                self.PdfHist()
            elif type=='fflux':
                pass
                
    def Load(self, mode='LOAD_FRESH_ONLY'):
        '''
        loads an .lmint file
        returns True if load is successful, False otherwise
        possible modes - 
        LOAD_FRESH_ONLY: will only load the .lmint file if a call to CheckMod returns True
        LOAD_OLD: will load the .lmint file as long as it exists
        '''
        print('attempting to load intermediate file for sweep datapoint %s...' % self)
        if mode=='LOAD_FRESH_ONLY':
            if not self.CheckMod():
                print('...load failed due to out of date or non-existant mod file'); return False
            try:
                self._Load()
            except OSError:
                print('...load failed due to OSError raised in internal _Load function'); return False
            print('...load successful'); return True
        elif mode=='LOAD_OLD':
            try:
                self._Load()
            except OSError:
                print('...load failed due to OSError raised in internal _Load function'); return False
            print('...load successful'); return True
        else:
            print('...load failed due to unknown mode argument value: %s' % mode); return False
                
    def Save(self):
        with h5py.File(self.intermediatePath, 'w') as interF:
            if 'Parameters' not in self.inerF.keys():
                interF.create_group('Parameters')
            interF['Parameters'].attrs['type'] = self.type
            if self.type=='replicateFPT':
                
                pass
            elif self.type=='replicateProbability':
                simulationsGroup = interF.create_group("/Simulations")
                for i,key in enumerate(self.replicateKeys):
                    replicateGroup = interF.create_group("/Simulations/%07d" % int(key))
                    # save out histogram binned output from each replicate as returned by matplotlib's hist2d function
                    histogram = replicateGroup.create_dataset("Histogram", self.histogram[i].shape, dtype=np.dtype('i32'))
                    histogram[...] = self.histogram[i]
                    xBinCoordinates = replicateGroup.create_dataset("XBinCoordinates", self.xBinCoordinates[i].shape, dtype=np.dtype('i32'))
                    xBinCoordinates[...] = self.xBinCoordinates[i]
                    yBinCoordinates = replicateGroup.create_dataset("YBinCoordinates", self.yBinCoordinates[i].shape, dtype=np.dtype('i32'))
                    yBinCoordinates[...] = self.yBinCoordinates[i]
            elif self.type=='fflux':
                pass
            
        self.SaveMod()
    
    def Figname(self, suffix, ext=True):
        if ext:
            extension = '.png'
        else:
            extension = ''
        return os.path.join(self.fDir, self.fName + suffix + extension)
    
    def Pdf(self, equilibriate=False):
        self.oparam = np.array([])
        for sim in self.sims:
            sim.Pdf()
            if equilibriate:
                self.oparam = np.hstack([self.oparam, sim.oparam[len(sim.oparam)/10:]])
            else:
                self.oparam = np.hstack([self.oparam, sim.oparam])
    
    def PdfHist(self):
        histogram1D = {}
        for i in range(self.histogram.shape[0]):
            for j in range(self.histogram.shape[1]):
                count = self.histogram[i,j] 
                delta = self.yBinCoordinates[j] - self.xBinCoordinates[i]
                histogram1D[delta] = count + histogram1D.get(delta, 0)
        self.histogram1D = np.zeros((len(histogram1D.keys()),2))
        for i,(key,val) in enumerate(sorted(histogram1D.items())):
            self.histogram1D[i,0] = key
            self.histogram1D[i,1] = val

    
    def FFluxProb(self, axes=None, fig=None, s=100):
        if axes==None:
            axes = plt.axes()
        if fig==None:
            fig = plt.figure(1)
            fig.set_size_inches(24,24)
        
        print(self.normalizedProbabilityI)
        print(np.linspace(-22.5,22.5,12),)
        normP = self.normalizedProbabilityI[1:-1]
        im = axes.scatter(np.linspace(-22.5,22.5,12), normP, c='g', s=s)
        
        axes.set_yscale('log')
        
        matplotlib.rcParams.update({'font.size': 20})
        axes.set_title('Rate scaling=%.2f' % self.sweepParams['production'], fontsize=14)
#         axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))', fontsize=14)
        axes.set_xlim(-25,25)
        axes.set_xticks((-23,-10,0,10,23))
        axes.set_ylim(5e-6, 5e-1)
        
        fontSizeTicks = 14
        for xtick in axes.xaxis.get_major_ticks():
            xtick.label.set_fontsize(fontSizeTicks)
        for ytick in axes.yaxis.get_major_ticks():
            ytick.label.set_fontsize(fontSizeTicks)
        
    def Progress(self):
        progs = []
        for sim in self.sims:
            progs.append(sim.Progress(self.maxtime))
        return np.mean(progs)
    
    def Rcoords(self, sim):
        self.rcoords = np.array([[],[]])
        sim.Rcoords()
        self.rcoords = np.hstack([self.rcoords, np.sum(sim.rcoords,0)])

    def RcoordsHist(self, sim):
        self.rcoords = np.array([[],[]])
        sim.Rcoords()
        h, x, y, p = plt.hist2d(sim.rcoords[0,:], sim.rcoords[1,:], range=[[0,100],[0,100]], bins=(100, 100))
        self.histogram.append(h); self.xBinCoordinates.append(x); self.yBinCoordinates.append(y); #self.p.append(p);
        
    def Savefig(self, fig, suffix):
        fname = self.Figname(suffix)
        fig.savefig(fname, bbox_inches='tight', transparent=True)
    
    def Hist(self, axes=None, fig=None, fontSize=20, fontSizeTicks=14, hideLastTick=False, logScaled=False, normed=False, normedRange=(-22.5,22.5), saveFig=True):
        if axes==None:
            axes = plt.axes()
        if fig==None:
            fig = plt.figure(1)
            fig.set_size_inches(24,24)
            
        if normed:
            if normedRange:
#                 leftIndex = np.searchsorted(self.histogram1DNormed[:,0], normedRange[0])
#                 rightIndex = np.searchsorted(self.histogram1DNormed[:,0], normedRange[1])
                #self.histogram1DNormed[leftIndex:rightIndex,:]
                self.histogram1D = self.histogram1D[74:125,:]
#             print(self.histogram1DNormed)
            self.histogram1DNormed = np.copy(self.histogram1D)
            self.histogram1DNormed[:,1] = self.histogram1D[:,1]/np.sum(self.histogram1D[:,1])
            histToPlot = self.histogram1DNormed
        else:
            histToPlot = self.histogram1D
            
        line = axes.plot(histToPlot[:,0], histToPlot[:,1])
        
        if logScaled==True:
            axes.set_yscale('log')
        
        matplotlib.rcParams.update({'font.size': fontSize})
        axes.set_title('Rate scaling=%.2f' % self.sweepParams['production'], fontsize=14)
#         axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))', fontsize=14)
        axes.set_xlim(-25,25)
        axes.set_xticks((-23,-10,0,10,23))
        axes.set_ylim(5e-6, 5e-1)
        
        for xtick in axes.xaxis.get_major_ticks():
            xtick.label.set_fontsize(fontSizeTicks)
        for ytick in axes.yaxis.get_major_ticks():
            ytick.label.set_fontsize(fontSizeTicks)
        if hideLastTick:
            plt.setp(xtick.label, visible=False)
            plt.setp(ytick.label, visible=False)
            
        if saveFig:
            self.Savefig(fig, '_hist')
            
        return line
    
    def HistClassic(self):
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        n, bins, patches = axes.hist(self.oparam, bins=200, range=(-100,100), normed=True)
        axes.plot(bins[1:] - .5, n, 'r--')  # bins is the x coord of each vertical line on the histogram. n is the height of each bin. Thus, len(bins) = len(n)+1, and so the bins[1:] weirdness
        fig.set_size_inches(36,24)
        axes.set_xticks(range(-100,101,5))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        labels = ax.get_yticks().tolist()
        labels = ['']*len(labels)
        ax.set_yticklabels(labels)
        self.Savefig(fig, '_hist')
        print('done')
    
    def HistUmbrella(self):
        '''histogram with umbrellas on for explaining umbrella sampling'''
        frames = 100
        playbacktime = 3
        denominator = 2000
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        fig.set_size_inches(12,8)
        ax = plt.axes()
        print('graphing now...')
        n, bins, patches = ax.hist(self.oparam, bins=200, range=(-100,100), normed=True)
        line_x = bins[1:] - .5   # bins is the x coord of each vertical line on the histogram. n is the height of each bin. Thus, len(bins) = len(n)+1, and so the bins[1:] weirdness 
        ylim = (1.05*np.max(n))
        parabola_range = np.product(np.sqrt((denominator, ylim)))
        l = len(range(-70,71,10)) 
        aboves = (np.array(range(l))*.5+2)*ylim
        aboves[1:] = aboves[1:] #+ 100
        np.random.shuffle(aboves[1:])   # randomize all of the aboves values except the first, for a nice random umbrella sampling
        aboves[0],aboves[2] = aboves[2],aboves[0]   # move the first umbrella to fall towards the center a little bit, because it looks nicer
        aboves[2] = ylim    # the first umbrella will have already fallen into place with this line
        
        def animate(frame):
            ax.cla()
#             ax.plot(line_x, n, 'k', linewidth=8)    
            for i,start in enumerate(range(-70,71,10)):
                # stuff to fill in the line segments when the parabola is finished descending
                if aboves[i]<=ylim:
                    line_start = bi.bisect(line_x, start - parabola_range)
                    line_end = bi.bisect(line_x, start + parabola_range) - 1
                    ax.plot(line_x[line_start:line_end], n[line_start:line_end], 'k', linewidth=8)
                x = np.linspace(start - parabola_range,start + parabola_range,400)
                ax.plot(x, -((x-start)**2)/denominator + aboves[i], 'b', linewidth=2)
                if aboves[i] > ylim:
                    aboves[i] = aboves[i] - .1*ylim
                if aboves[i] < ylim:
                    aboves[i] = ylim
            ax.set_xticks(range(-100,101,10))
            ax.set_xlim((-80,80))
            ax.set_ylim((0,ylim))
            ax.set_xlabel('$\Delta$ (copy num(b) - copy num(a))', fontsize=30)
            ax.set_ylabel('normalized count', fontsize=30)
            ax.invert_yaxis()
#             if frame==99:
#                 self.Savefig(fig, '_histumbrella_static_last')
            print("saving frame _%09d" % frame)
        
#         [animate(9999) for foo in range(100)]
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('hist_umbrella_1st_1_animation.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print('done')
    
    def Hist2D(self, axes=None, fig=None, fontSize=20, fontSizeTicks=14, fontSizeTitle=14, hideLastTick=False, norm=None, plotCbar=False, saveFig=True):
        if axes==None:
            axes = plt.axes()
        if fig==None:
            fig = plt.figure(1)
            fig.set_size_inches(24,24)
        self.normedHistogram = np.copy(self.histogram)
        self.normedHistogram = self.normedHistogram/(np.max(self.histogram))
        im = axes.imshow(self.normedHistogram, aspect='equal', extent=(0,100,0,100), norm=norm, origin='lower', cmap=cm.hot_r)
        if plotCbar:
            l_f = LogFormatter(10, labelOnlyBase=False)
            cbar = fig.colorbar(im, ticks=np.logspace(-7,-1,6), format=l_f)
        
        axes.set_title('Rate scaling=%.2f' % self.sweepParams['production'], fontsize=fontSizeTitle)
        matplotlib.rcParams.update({'font.size': fontSize})
#         axes.set_xlabel('copy num(A)')
#         axes.set_ylabel('copy num(B)')
        for xtick in axes.xaxis.get_major_ticks():
            xtick.label.set_fontsize(fontSizeTicks)
        for ytick in axes.yaxis.get_major_ticks():
            ytick.label.set_fontsize(fontSizeTicks)
        if hideLastTick:
            plt.setp(xtick.label, visible=False)
            plt.setp(ytick.label, visible=False)
            
        if saveFig:
            self.Savefig(fig, '_hist2d')
        
    def Hist2DSepTraj(self):
        '''
        plots each individual trajectory as a separate Hist2D
        '''
        if self.rcoords==None:
            self.Rcoords()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        for i,sim in enumerate(self.sims):
            axes.hist2d(sim.rcoords[0,:], sim.rcoords[1,:], range=[[0,100],[0,100]], bins=(100, 100))
            fig.set_size_inches(36,24)
            matplotlib.rcParams.update({'font.size': 42})
            axes.set_xlabel('copy num(A)')
            axes.set_ylabel('copy num(B)')
            self.Savefig(fig, '_hist2d_replicate%03d' % i)
            print('done')
        
    def Hist2DLog(self):
        if self.rcoords==None:
            self.Rcoords()
        fig = plt.figure(1)
        fig.set_size_inches(8,8)
        axes = plt.axes()
        axes.set_aspect('equal')
        print('graphing now...')
        counts,xbins,ybins,image=plt.hist2d(self.rcoords[0,:], self.rcoords[1,:],range=((0,100),(0,100)), bins=(100,100), normed=True, norm=LogNorm(), cmap=cm.hot_r)# cmap=pCMap['cube1']) #cmap=cm.hot, range=[[0,84],[0,84]], bins=(84, 84))
#         plt.colorbar()

        matplotlib.rcParams.update({'font.size': 30})
        axes.set_xlabel('copy num(A)')
        axes.set_ylabel('copy num(B)')
#         start, end = axes.get_xlim()
#         axes.xaxis.set_ticks(np.arange(start, end, 20))
#         start, end = axes.get_ylim()
#         axes.yaxis.set_ticks(np.arange(start, end, 20))
        self.Savefig(fig, '_hist2d_log_perceptual')
        
        counts,xbins,ybins,image =plt.hist2d(self.rcoords[0,:], self.rcoords[1,:],range=((0,40),(0,40)), bins=(40,40), normed=True, cmap=pCMap['cube1'])
        fig.clf()
        axes = plt.axes()
        matplotlib.rcParams.update({'font.size': 30})
        colorline([a-b for a,b in zip(range(0,26),range(25,-1,-1))], [counts[x,y] for x,y in zip(range(0,26),range(25,-1,-1))],[counts[x,y] for x,y in zip(range(0,26),range(25,-1,-1))],ax=axes)
        
        axes.set_xlim(-25,25)
        axes.set_ylim(.1,-1)
        axes.invert_yaxis()
        matplotlib.rcParams.update({'font.size': 30})
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
#         labels = axes.get_yticks().tolist()
#         labels = ['']*len(labels)
#         axes.set_yticklabels(labels) 
        self.Savefig(fig, '_hist2d_log_perceptual_1d')
        print('done')
    
    def Hist2DLogBounce(self):
        if self.rcoords==None:
            self.Rcoords()
        datapoints = 10000
        frames = 400
        playbacktime = 15   # in seconds
        step = datapoints/frames
        fig = plt.figure(1)
        fig.set_size_inches(12,8)
        axes = plt.axes()
        print('graphing now...')
        plt.hist2d(self.rcoords[0,:], self.rcoords[1,:], range=[[0,84],[0,84]], bins=(84, 84),normed=True, norm=LogNorm(), cmap=pCMap['cube1'])
        plt.colorbar()
        bb = Circle((0.5, 0.5), 1, fc='r')    # bb, the bouncing ball! star of the show
        matplotlib.rcParams.update({'font.size': 30})
        axes.set_xlabel('copy num(A)')
        axes.set_ylabel('copy num(B)')
        start, end = axes.get_xlim()
        axes.xaxis.set_ticks(np.arange(start, end, 20))
        start, end = axes.get_ylim()
        axes.yaxis.set_ticks(np.arange(start, end, 20))
        axes.add_artist(bb)
        def animate(frame):
            print("on frame %04d" % frame)
            bb.center = self.rcoords[:,frame*step]
        
        plt.subplots_adjust(left=0.15, right=.95, bottom=.15, top=.95)
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('bouncing_ball.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print('done')
    
    def Passage(self):
        self.dataCount = 0
        self.dwells = np.array([])
        for sim in self.sims:
            self.dwells = np.hstack([self.dwells, sim.Passage()])
            self.dataCount+=len(sim.oparam)
        print('total normalized data points recorded for this set of replicates: %.8f' % (self.dataCount/float(10**6)))
        if len(self.dwells)>0:
            #fit dwell times to exponential
            def func(x, k): return 1-np.exp(-x*k())
            init = {'k':0}
            xdata = np.array(sorted(self.dwells.tolist()))
            ydata = np.array(range(1, len(self.dwells) + 1))/float(len(self.dwells))
            fit = Fit(func, init, xdata=xdata, ydata=ydata)
            print('graphing now...')
            fit.ScatterPlot(self.Figname('_passage_cdf', ext=False), clear=True)
            fit.SmoothPlot(self.Figname('_passage_cdf', ext=False), lim=('auto','auto'), labels=('time (k)','cdf'), annotate='k: %10e' % fit.params['k'].value)
            print('done')
            print(fit)
            print(self.dwells)
            print('mean: %.10f' % np.mean(self.dwells))
#             kmf = KaplanMeierFitter()
#             kmf.fit(self.dwells.tolist())
#             print(kmf)
#             print(dir(kmf))
#             self.cf = CoxPHFitter()
#             dataFrame = pd.DataFrame(self.dwells)
#             try:
#                 self.cf.fit(dataFrame, 0)
#                 print('b0: %s' % self.cf.baseline_hazard_)
#                 print('b1: %s' % self.cf.hazards_)
#             except ValueError:
#                 pass
        print('The normalized count of switching events: %d' % len(self.dwells))
    
    def Surf3D(self):
        xbincnt = 300
        ybincnt = 300
        if self.rcoords==None:
            self.Rcoords()
        fig = plt.figure(1)
        counts,xbins,ybins,image = plt.hist2d(self.rcoords[0,:], self.rcoords[1,:], range=[[0,60],[0,60]],normed=True, norm=LogNorm(), bins=(xbincnt, ybincnt), cmap=pCMap['cube1'])
        fig.clf()
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        x,y = np.zeros((xbincnt,ybincnt)),np.zeros((xbincnt,ybincnt))
        x[:,:] = xbins[:-1]+.5; y[:,:] = ybins[:-1]+.5
        x = x.T
#         X = np.arange(-5, 5, 0.25)
#         Y = np.arange(-5, 5, 0.25)
#         X, Y = np.meshgrid(X, Y)
#         R = np.sqrt(X**2 + Y**2)
#         Z = np.sin(R)
#         Z=Z*0
        ax.set_zscale('log')
        surf = ax.plot_surface(x, y, counts, rstride=1, cstride=1, cmap=rPCMap['cube1'],
                linewidth=0, antialiased=False, alpha=.3, norm=LogNorm())
        ax.set_zlim(0, .04)
        
        plt.show()

#         plt.show()
#         x,y = np.zeros((xbincnt,ybincnt)),np.zeros((xbincnt,ybincnt))
#         x[:,:] = xbins[:-1]+.5; y[:,:] = ybins[:-1]+.5
#         x = x.T
#         fig.clf()
#         fig.set_size_inches(12,8)
#         ax = fig.gca(projection='3d')
#         ax.xaxis._axinfo['label']['space_factor'] = ax.yaxis._axinfo['label']['space_factor'] = ax.zaxis._axinfo['label']['space_factor'] = .5
#         
#         ax.plot_surface(x, y, 1, linewidth=1, rstride=1, cstride=1, cmap=rPCMap['cube1'], antialiased=True, alpha=1, shade=True)
#         fig.savefig('here.png', bbox_inches='tight', transparent=True)
       
    def Tcourse(self, i=0):
        if self.sims[i].oparam==None:
            self.sims[i].Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        axes.plot(self.sims[i].times[::100], self.sims[i].oparam[::100])
        fig.set_size_inches(72,6)
        axes.set_yticks(range(-100,101,10))
        axes.set_xlabel('time (k)')
        axes.set_ylabel('$\Delta$ (copy num(b) - copy num(a))')
        self.Savefig(fig, '_tcourse')
        print('done')
          
    def TrajMov(self, trajID=0):
        datapoints = 10000
        frames = 400
        playbacktime = 15
        step = datapoints/frames
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        fig.set_size_inches(12,8)
#         fig.patch.set_facecolor('white')
#         fig.patch.set_alpha(0)
        axes = plt.axes()
        print('graphing now...')
        xoparams = self.sims[trajID].oparam[:datapoints]
        
        def animate(frame):
            axes.cla()
#             axes.patch.set_facecolor('white')
#             axes.patch.set_alpha(0)
            oparam = xoparams[frame*step]
            nCum, binsCum, patchesCum = axes.hist(xoparams[:(frame*step)+1], bins=200, range=(-100,100), normed=True)
            idx = np.argmin(np.abs(binsCum[1:] - .5 - oparam))
#             yoparamNow = interpolate.splev([oparam], tckCum, der=0)
#             dot = axes.scatter([oparam],[yoparamNow[0]],s=100,c='g')
            dot = axes.scatter([oparam],[nCum[idx]],s=100,c='r')           
            axes.set_xticks(range(-100,101,10))
            axes.set_xlim((-80,80))
            axes.set_ylim((0,1.05*np.max(nCum)))
            axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))', fontsize=30)
            axes.set_ylabel('normalized count', fontsize=30)
            axes.invert_yaxis()
            print("saving frame _%09d" % frame)
        
        plt.subplots_adjust(left=0.1, right=.95, bottom=.15, top=.95)
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('mov_traj.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print('done')
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4
    
    def TrajMovSmooth(self, trajID=0):
        frames = 100
        playbacktime = 3
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        xoparam = self.sims[trajID].oparam[0:frames]
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
        def animate(frame):
            oparam = xoparam[frame]
            nCum, binsCum, patchesCum = axes.hist(xoparam[:frame+1], bins=200, range=(-100,100))#, normed=True)
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
#                 self.Savefig(fig, '_%09d' % i)
            print("saving frame _%09d" % frame)
            dot.remove()
        
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('smooth_mov_traj.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print('done')
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -vcodec png biphasic_switch_direct_3spline.mov
        
class Biphasic(Sim):
    basindict = {'A':0,'B':1}

    def Basin(self, framei):
        if self.oparam[framei] < -25:
            return 'A'
        elif self.oparam[framei] > 25:
            return 'B'
        else:
            return False

    def Pdf(self, limitToTravelTime=False):
        reducer = np.array([-1,-2,-2,1,2,2,0])
        if limitToTravelTime:
            timeStepCount = bi.bisect(self.times, self.sweepParams['traveltime'])
            self.oparam = np.dot(self.counts[:timeStepCount], reducer)
        else:
            self.oparam = np.dot(self.counts, reducer)
            
    def Rcoords(self):
        '''
        reaction coordinates
        '''
        reducer = np.array([[1,2,2,0,0,0,0],[0,0,0,1,2,2,0]]).T
        self.rcoords = np.dot(self.counts, reducer).T
        
    def TransitionProbability(self):
        '''
        if this trajectory transitioned from one basin to another, return 1. otherwise return 0
        '''
        if self.Basin(0)!=self.Basin(len(self.oparam) - 1):
            return 1
        else:
            return 0

class Biphasics(Sims):
    child = Biphasic
    
    def Ratio(self):
        return float(self.sweepParams['production'])/float(self.sweepParams['degradation'])
    
    def TransitionCount(self):
        '''
        return the count of replicates in this .lm file that transitioned from one basin to another
        '''
        if self.oparam==None:
            self.Pdf()
        transitionCount = np.sum([sim.TransitionProbability() for sim in self.sims])
        return int(transitionCount)

class SweepBiphasics(Biphasics):
    def __init__(self, rootPath, type='replicateProbability', unpickle=False):
        self.fname = rootPath.split('/')[-1]
        self.simsSweep = []
        self.simsSweepDict = {}
        self.sweepParams = {}
        for tup in os.walk(rootPath):
            lmFName = None
            lmintFName = None
            # the following loop expects there to be at most one .lm file and one .lmint file in a directory
            for fname in tup[2]:
                if fname[-3:]=='.lm':
                    lmFName = fname
                elif fname[-6:]=='.lmint':
                    lmintFName = fname
                    
                if lmFName or lmintFName:
                    # the following regex captures things of the form (name)(float-val)_(name)(float-val)
                    sweepParamRe = re.search('([^\W\d_]+)([\d\.]+)_([^\W\d_]+)([\d\.]+)', tup[0])
    #                     sweepParamTups = zip(sweepParamRe.groups()[::2],sweepParamRe.groups()[1::2])
    #                     if float(sweepParamRe.group(2))==1.0 and float(sweepParamRe.group(4))==0.25:
                    sweepParamDict = {sweepParamRe.group(1): float(sweepParamRe.group(2)),
                                      sweepParamRe.group(3): float(sweepParamRe.group(4))}
                    for key,val in sweepParamDict.items():
                        tmpSweepParamsList = self.sweepParams.get(key, [])
                        if val not in tmpSweepParamsList:
                            self.sweepParams[key] = sorted(tmpSweepParamsList + [val])
                            
                if lmFName:
                    lmFPath = os.path.join(tup[0], lmFName)
                    self.simsSweep.append(Biphasics(lmFPath, type, **sweepParamDict))
                    self.simsSweepDict[(float(sweepParamRe.group(2)), float(sweepParamRe.group(4)))] = self.simsSweep[-1]
                    print('finished intermediate processing of sweep datapoint %s' % self.simsSweep[-1])
                elif lmintFName:
                    # we have reached this point because there was only an .lmint file in the directory and no .lm file
                    lmintFPath = os.path.join(tup[0], lmintFName)
                    paramString = (' %s:%s'*(len(sweepParamRe.groups())//2)) % sweepParamRe.groups()
                    print('no .lm file for sweep datapoint%s, loading from .lmint file' % paramString)
                    self.simsSweep.append(Biphasics(lmintFPath, lmintOnly=True, **sweepParamDict))
                    self.simsSweepDict[(float(sweepParamRe.group(2)), float(sweepParamRe.group(4)))] = self.simsSweep[-1]
        self.simsSweep.sort(key=lambda x: x.sweepParams[sweepParamRe.group(1)])
    
    def CombineReplicateProbabilitySweeps(self, other):
        '''
        self and other should be of type ReplicateSweep
        '''
        for key in self.simsSweepDict.keys():
            if key in other.simsSweepDict.keys():
                print('key %s:%s found in other ReplicateSweep' % (key[0], key[1]))
                self.simsSweepDict[key].histogram+=other.simsSweepDict[key].histogram
                self.simsSweepDict[key].PdfHist()
    
    def FFluxProb(self, s=100, figDiagonalLength=6, fontSize=20, fontSizeTicks=14, hideLastTick=False, hideRedundantAxes=True, 
                  hspace=.1, logScaled=False, normed=False, saveFig=True, shape=None, wspace=.1):
        self.linesForLegend = []
        if shape==None:
            shape = [len(self.simsSweep)]
        
        fig,axesArr = plt.subplots(*shape)#, sharex=True, sharey=True)
        fig.set_size_inches(shape[1]*figDiagonalLength*2**(-1/float(2)), shape[0]*figDiagonalLength*2**(-1/float(2)))
        for i,sims in enumerate(self.simsSweep[::4]):
            sims.FFluxProb(axes=axesArr.ravel()[i], fig=fig, s=s)
        
        # delete the unused axes
        i+=1
        while i < axesArr.size:
#             sims.Hist2D(axes=axesArr.ravel()[i], fig=fig, fontSize=fontSize, saveFig=False)
            axesArr.ravel()[i].axis('off')
            fig.delaxes(axesArr.ravel()[i])
            i+=1
            
        fig.subplots_adjust(hspace=hspace)
        fig.subplots_adjust(wspace=wspace)
        if hideRedundantAxes:
            for i in range(shape[0]-1):
                for j in range(shape[1]):
                    axesArr[i,j].set_xlabel('')
                    plt.setp(axesArr[i,j].get_xticklabels(), visible=False)
            
            for i in range(shape[0]):
                for j in range(1,shape[1]):
                    axesArr[i,j].set_ylabel('')
                    plt.setp(axesArr[i,j].get_yticklabels(), visible=False)
                    
        return axesArr, fig
        
    def Hist(self, figDiagonalLength=6, fontSize=20, fontSizeTicks=14, hideLastTick=False, hideRedundantAxes=True, 
             hspace=.1, logScaled=False, normed=False, saveFig=True, shape=None, wspace=.1):
        
        self.linesForLegend = []
        if shape==None:
            shape = [len(self.simsSweep)]
        fig,axesArr = plt.subplots(*shape)#, sharex=True, sharey=True)
        fig.set_size_inches(shape[1]*figDiagonalLength*2**(-1/float(2)), shape[0]*figDiagonalLength*2**(-1/float(2)))
        for i,sims in enumerate(self.simsSweep):
            line = sims.Hist(fontSize=fontSize, fontSizeTicks=fontSizeTicks, hideLastTick=hideLastTick, logScaled=logScaled, normed=normed, saveFig=saveFig)
            self.linesForLegend.append(line)
            print(sims.sweepParams.keys())
            print(sims.sweepParams.values())
            
#         fig.text(0.5, 0.04, 'common X', ha='center')
#         fig.text(0.04, 0.5, 'common Y', va='center', rotation='vertical')
#         fig.set_xlabel('$\Delta$ (copy num(b) - copy num(a))', fontsize=14)
#         fig.set_ylabel()
        
        # delete the unused axes
        i+=1
        while i < axesArr.size:
            sims.Hist2D(axes=axesArr.ravel()[i], fig=fig, fontSize=fontSize, saveFig=False)
            axesArr.ravel()[i].axis('off')
            fig.delaxes(axesArr.ravel()[i])
            i+=1
        # hack to delete the penulous extra 11th plot
#         axesArr.ravel()[10].axis('off')
#         fig.delaxes(axesArr.ravel()[10])
        
        # squish the subplots together
        fig.subplots_adjust(hspace=hspace)
        fig.subplots_adjust(wspace=wspace)
        if hideRedundantAxes:
            for i in range(shape[0]-1):
                for j in range(shape[1]):
                    axesArr[i,j].set_xlabel('')
                    plt.setp(axesArr[i,j].get_xticklabels(), visible=False)
            
            for i in range(shape[0]):
                for j in range(1,shape[1]):
                    axesArr[i,j].set_ylabel('')
                    plt.setp(axesArr[i,j].get_yticklabels(), visible=False)
    
        # add legend
        
#         fig.legend((self.linesForLegend[-1],), ('Probability determined\nbyreplicate sampling',), 'lower right')
        
        return axesArr, fig
        
    def Hist2D(self, figDiagonalLength=6, fontSize=20, fontSizeTicks=14, fontSizeTitle=14, hideLastTick=False, hspace=.1, norm=None, plotCbarAtEnd=False, saveFig=True, shape=None, wspace = .1):
        plotCbarAtEnd = True
        print(plotCbarAtEnd)
        if shape==None:
            shape = [len(self.simsSweep)]
        fig,axesArr = plt.subplots(*shape)#, sharex=True, sharey=True)
        fig.set_size_inches(shape[1]*figDiagonalLength*2**(-1/float(2)), shape[0]*figDiagonalLength*2**(-1/float(2)))
        for i,sims in enumerate(self.simsSweep):
            if plotCbarAtEnd and i==shape[0]-1:
                plotCbar = True
            else:
                plotCbar = False
            sims.Hist2D(axes=axesArr.ravel()[i], fig=fig, fontSize=fontSize, fontSizeTicks=fontSizeTicks, fontSizeTitle=fontSizeTitle, 
                        hideLastTick=hideLastTick, norm=norm, plotCbar=plotCbar, saveFig=saveFig)
            print(sims.sweepParams.keys())
            print(sims.sweepParams.values())
            
        # delete the unused axes
        i+=1
        while i < axesArr.size:
            sims.Hist2D(axes=axesArr.ravel()[i], fig=fig, fontSize=fontSize, norm=None, plotCbar=False, saveFig=False)
            axesArr.ravel()[i].axis('off')
            fig.delaxes(axesArr.ravel()[i])
            i+=1
        # squish the subplots together
        fig.subplots_adjust(hspace=hspace)
        fig.subplots_adjust(wspace=wspace)
        for i in range(shape[0]-1):
            for j in range(shape[1]):
                axesArr[i,j].set_xlabel('')
                plt.setp(axesArr[i,j].get_xticklabels(), visible=False)
        
        for i in range(shape[0]):
            for j in range(1,shape[1]):
                axesArr[i,j].set_ylabel('')
                plt.setp(axesArr[i,j].get_yticklabels(), visible=False)
        
        return fig
    
    def Hist2D_Separate(self):
        for sims in self.simsSweep:
            sims.Hist2D()
            print(sims.sweepParams.keys())
            print(sims.sweepParams.values())
            del sims
    
    def Hist2DLog_Separate(self):
        for sims in self.simsSweep:
            sims.Hist2DLog()
            print(sims.sweepParams.keys())
            print(sims.sweepParams.values())
            del sims
            
    def Hist2DTransitionProbability(self):
        '''
        plots the fraction of trajectories that transitioned from one well to another in each bin of a parameter sweep
        '''
        transitions = []
        for sims in self.simsSweep:
            transitions+=[sims.sweepParams.values()]*sims.TransitionCount()
            sweepParamNames = sims.sweepParams.keys()
#             del sims
#         if self.rcoords==None:
#             self.Rcoords()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        rang = [[self.sweepParams[sweepParamNames[0]][0] - .00001, self.sweepParams[sweepParamNames[0]][-1] + (self.sweepParams[sweepParamNames[0]][-1] - self.sweepParams[sweepParamNames[0]][-2]) - .00001],
                [self.sweepParams[sweepParamNames[1]][0] - 1 , self.sweepParams[sweepParamNames[1]][-1] + (self.sweepParams[sweepParamNames[1]][-1] - self.sweepParams[sweepParamNames[1]][-2]) - 1]]
        bins = [len(self.sweepParams[sweepParamNames[0]]),
                len(self.sweepParams[sweepParamNames[1]])]
        hist = plt.hist2d(zip(*transitions)[0], zip(*transitions)[1], range=rang, bins=bins, cmap=cm.hot_r)
        plt.colorbar()
        xBinCenters = hist[1][:-1] + ((hist[1][1:] - hist[1][:-1])/2)
        yBinCenters = hist[2][:-1] + ((hist[2][1:] - hist[2][:-1])/2)
        normedHistArr = hist[0]/np.max(hist[0])
        print(normedHistArr)
        it = np.nditer([hist[0], normedHistArr], flags=['multi_index'])
        while not it.finished:
            print(it[1], cm.hot_r(it[1]), AnnotationColor(*cm.hot_r(it[1])))
            axes.annotate('%d' % it[0],
                          xy=(xBinCenters[it.multi_index[0]], yBinCenters[it.multi_index[1]]), 
                          color=AnnotationColor(*cm.hot_r(it[1])),
                          size=20,
                          horizontalalignment='center',
                          verticalalignment='center')
            it.iternext()
        
        fig.set_size_inches(36,24)
        matplotlib.rcParams.update({'font.size': 42})
        axes.set_xlabel(sweepParamNames[0])
        axes.set_ylabel(sweepParamNames[1])
        self.Savefig(fig, '_hist2d_transition_probability')
        print('done')

    def Passage(self):
        for sims in [sims for sims in self.simsSweep if np.isclose(sims.Ratio(), 4.0, atol=1e-01)]:
            print(sims)
            sims.Passage()
            sims.f.close()
            del sims.f
            del sims
    
    def Save(self):
        for sims in self.simsSweep:
            sims.Save()
    
class FFluxBiphasic(Biphasic):
    pass

class FFluxBiphasics(Biphasics):
    child = FFluxBiphasic
    
    def SortByInterface(self):
        if self.oparam==None:
            self.Pdf()
        self.interfaces = np.linspace(-25,25,13)
        self.interfaceDict = {}
        for sim in self.sims:
            self.interfaceDict[bi.bisect(self.interfaces,sim.oparam[0])] = self.interfaceDict.get(bi.bisect(self.interfaces,sim.oparam[0]), []) + [sim]
        self.interfaceDictSorted = OrderedDict(sorted(self.interfaceDict.items(), key=lambda x: x[0]))
            
    def TrajMovSmooth(self):
        if self.oparam==None:
            self.SortByInterface()
        fig = plt.figure(1)
        axes = plt.axes()
        print('graphing now...')
        
        #for 
        nAll, binsAll, patchesAll = axes.hist(xoparam, bins=len(self.interfaces)-1, range=(-100,100))#, normed=True)
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
                print("saving frame _%09d" % i)
                dot.remove()
        print('done')
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4    

if __name__=="__main__":
    rootPath = sys.argv[1]
#     ffluxbiphasics = FFluxBiphasics(fname)
#     ffluxbiphasics.SortByInterface()
#     ffluxbiphasics.TrajMovSmooth()

    sweepBiphasics = SweepBiphasics(rootPath)
#     if sys.argv[2]=='hist2d':
#         sweepBiphasics.Hist2D()
#     elif sys.argv[2]=='hist2dlog':
#         sweepBiphasics.Hist2DLog()
#     elif sys.argv[2]=='hist2dtp':
#         sweepBiphasics.Hist2DTransitionProbability()
#     elif sys.argv[2]=='passage':
#         sweepBiphasics.Passage()
#     sweepBiphasics.Save()