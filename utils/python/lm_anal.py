#!/usr/local/bin/python
import bisect as bi
from collections import OrderedDict
import h5py
from os import path
from mpl_toolkits.mplot3d import Axes3D
import matplotlib
#matplotlib.use('Agg')
from matplotlib import animation
from matplotlib.collections import PatchCollection, LineCollection
import matplotlib.cm as cm
from matplotlib.colors import LogNorm
from matplotlib.patches import Circle, Wedge, Polygon
import matplotlib.pyplot as plt
import numpy as np
import os
import pylab
from scipy import interpolate
import sys

from fit import Fit

#matplotlib.rcParams['savefig.transparent'] = True

cm.hot.set_bad(cm.hot(0))
# stuff to make the perceptual color maps
pCMap = {}
rPCMap = {}
walk = os.walk(os.path.join(os.path.dirname(os.path.realpath(__file__)),'colormaps')).next()
for fname in (fname for fname in walk[2] if fname[0]!='.'):
    with open(os.path.join(walk[0],fname)) as f:
        cVals = np.array([map(float,line.strip().split(',')) for line in f])
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
        RGB = zip(R,G,B)
        rgb = zip(*RGB)
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
    for spine in ax.spines.itervalues(): 
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
    
    def Pdf(self):
        pass
    
    def Progress(self, maxt):
        return self.times[-1]/float(maxt)
    
    def Rcoords(self):
        pass
        
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
    
    def Pdf(self, equilibriate=False):
        self.oparam = np.array([])
        for sim in self.sims:
            sim.Pdf()
            if equilibriate:
                self.oparam = np.hstack([self.oparam, sim.oparam[len(sim.oparam)/10:]])
            else:
                self.oparam = np.hstack([self.oparam, sim.oparam])
    
    def Progress(self):
        progs = []
        for sim in self.sims:
            progs.append(sim.Progress(self.maxtime))
        return np.mean(progs)
    
    def Rcoords(self):
        self.rcoords = np.array([[],[]])
        for sim in self.sims:
            sim.Rcoords()
            self.rcoords = np.hstack([self.rcoords, sim.rcoords])
    
    def Savefig(self, fig, suffix):
        fname = self.Figname(suffix)
        fig.savefig(fname, bbox_inches='tight', transparent=True)
    
    def Hist(self):
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
        n, bins, patches = axes.hist(self.oparam, bins=200, range=(-100,100), normed=True)
        axes.plot(bins[1:] - .5, n, 'r--')  # bins is the x coord of each vertical line on the histogram. n is the height of each bin. Thus, len(bins) = len(n)+1, and so the bins[1:] weirdness
        fig.set_size_inches(36,24)
        axes.set_xticks(range(-100,101,5))
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
        axes.set_ylabel('count')
        self.Savefig(fig, '_hist')
        print 'done'
    
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
        print 'graphing now...'
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
            print "saving frame _%09d" % frame
        
#         [animate(9999) for foo in range(100)]
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('hist_umbrella_1st_1_animation.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
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
        
    def Hist2DLog(self):
        if self.rcoords==None:
            self.Rcoords()
        fig = plt.figure(1)
        fig.set_size_inches(12,8)
        axes = plt.axes()
        print 'graphing now...'
        counts,xbins,ybins,image =plt.hist2d(self.rcoords[0,:], self.rcoords[1,:],range=((0,40),(0,40)), bins=(40,40), normed=True, norm=LogNorm(), cmap=pCMap['cube1']) #cmap=cm.hot, range=[[0,84],[0,84]], bins=(84, 84))
#         plt.colorbar()

        matplotlib.rcParams.update({'font.size': 30})
        axes.set_xlabel('copy num(A)')
        axes.set_ylabel('copy num(B)')
#         start, end = axes.get_xlim()
#         axes.xaxis.set_ticks(np.arange(start, end, 20))
#         start, end = axes.get_ylim()
#         axes.yaxis.set_ticks(np.arange(start, end, 20))
        self.Savefig(fig, '_hist2d_log_perceptual')
        
        fig.clf()
        axes = plt.axes()
        matplotlib.rcParams.update({'font.size': 30})
        
        print -np.array([counts[i,-i] for i in range(-25,26)])
        print [counts[i,-i] for i in range(-25,26)]
        
        colorline(range(-25,26), -np.array([counts[i,-i] for i in range(-25,26)]), [counts[i,-i] for i in range(-25,26)],ax=axes)
        
        axes.set_xlim(-25,25)
        #axes.set_ylim(0,-1)
        axes.invert_yaxis()
        matplotlib.rcParams.update({'font.size': 30})
        axes.set_xlabel('$\Delta$ (copy num(b) - copy num(a))')
#         labels = axes.get_yticks().tolist()
#         labels = ['']*len(labels)
#         axes.set_yticklabels(labels) 
        self.Savefig(fig, '_hist2d_log_perceptual_1d')
        print 'done'
    
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
        print 'graphing now...'
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
            print "on frame %04d" % frame
            bb.center = self.rcoords[:,frame*step]
        
        plt.subplots_adjust(left=0.15, right=.95, bottom=.15, top=.95)
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('bouncing_ball.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
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
        print 'graphing now...'
        axes.plot(self.sims[i].times[::100], self.sims[i].oparam[::100])
        fig.set_size_inches(72,6)
        axes.set_yticks(range(-100,101,10))
        axes.set_xlabel('time (k)')
        axes.set_ylabel('$\Delta$ (copy num(b) - copy num(a))')
        self.Savefig(fig, '_tcourse')
        print 'done'
          
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
        print 'graphing now...'
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
            print "saving frame _%09d" % frame
        
        plt.subplots_adjust(left=0.1, right=.95, bottom=.15, top=.95)
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('mov_traj.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print 'done'
        # to compile movie
        # ffmpeg -r 30 -i biphasic_switch_direct_%09d.png -c:v libx264 -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" -r 30 -pix_fmt yuv420p biphasic_switch_direct.mp4
    
    def TrajMovSmooth(self, trajID=0):
        frames = 100
        playbacktime = 3
        if self.oparam==None:
            self.Pdf()
        fig = plt.figure(1)
        axes = plt.axes()
        print 'graphing now...'
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
            print "saving frame _%09d" % frame
            dot.remove()
        
        anim = animation.FuncAnimation(fig, animate, frames=frames)
        anim.save('smooth_mov_traj.gif' ,writer='imagemagick', fps=frames/float(playbacktime));
        print 'done'
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

    def Pdf(self):
        reducer = np.array([-1,-2,-2,1,2,2,0])
        self.oparam = np.dot(self.counts, reducer)
    
    def Rcoords(self):
        reducer = np.array([[1,2,2,0,0,0,0],[0,0,0,1,2,2,0]]).T
        self.rcoords = np.dot(self.counts, reducer).T

class Biphasics(Sims):
    child = Biphasic

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
        print 'graphing now...'
        
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
    elif sys.argv[2]=='histumbrella':
        biphasics.HistUmbrella()
    elif sys.argv[2]=='hist2d':
        biphasics.Hist2D()
    elif sys.argv[2]=='hist2dlog':
        biphasics.Hist2DLog()
    elif sys.argv[2]=='hist2dlogbounce':
        biphasics.Hist2DLogBounce()
    elif sys.argv[2]=='passage':
        biphasics.Passage()
    elif sys.argv[2]=='surf3d':
        biphasics.Surf3D()
    elif sys.argv[2]=='tcourse':
        biphasics.Tcourse()
    elif sys.argv[2]=='trajmov':
        biphasics.TrajMov()
    elif sys.argv[2]=='trajmovsmooth':
        biphasics.TrajMovSmooth()