#from ..io.simFile import simFileFactory
# hdf5File = simFileFactory()
import os
import re

from ..helper import CamelCase, ShortenName, Singular
from ..oparam.oparams import OParams
from ..plottable import *
from src.datum.trajectory.replicateTrajectories import ReplicateTrajectories
from ..tiling.tilings import Tilings


class Sim(object):
    dataTypes = [OParams, ReplicateTrajectories, Tilings]
    plottableContainerTypes = [OParamProbabilityHists, OParamTrajectories]
    
    def __init__(self, fPath, dataTypes=None, lmintOnly=False, **kwargs):
        if dataTypes!=None:
            self.dataTypes = dataTypes
#         self.plottables = {}
            
        # path setting stuff
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.modTimePath = os.path.join(self.fDir, '.' + self.fName) + '.mod'
        self.intermediatePath = os.path.join(self.fDir, self.fName) + '.lmint'
        
        self.InitPlottableContainers()
        
        if lmintOnly:
            # we only have a .lmint file and no base .lm file
            self._Load()
        elif not self.Load():
            self.Init()
#         self.f = h5py.File(self.fPath)
#         self.params = self.f['/Parameters'].attrs
#         self.maxtime = self.params['maxTime']
#         self.writeinterval = self.params['writeInterval']
#         self.replicateKeys = list(self.f['/Simulations'].keys())
#         self.replicateCount = len(self.replicateKeys)
#         
#         self.tileEdges = self.f['/Tilings/0000000/Edges']
#         self.normalizedProbabilityI = self.f['/Tilings/0000000/FFluxOutput/NormalizedProbabilityI/TileVals']
            
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
        self.InitData()
    
    def InitData(self):
        self.dataDict = {}
        for dataObject in self.dataTypes:
            temp = dataObject(fPath=self.fPath)
            if temp.has():
                temp.rff()
                attrName = CamelCase(dataObject.__name__)
                self.__setattr__(attrName, temp)
                self.dataDict[attrName] = self.__getattribute__(attrName)
    
#     def _InitPlottable(self, id, type, **kwargs):
#         self.plottables[id] = plottable.__getattribute__(type)(self, **kwargs)
#     
#     def InitPlottable(self, id, type, useInt=True, **kwargs):
#         self._InitPlottable(id, type, **kwargs)
#         if useInt:
#             self.plottables[id].rff()
#         else:
#             self.plottables[id].transformData()
    
    def InitPlottable(self, id, type, useInt=True, **kwargs):
        for iP in self.initPlottables.values():
            if iP(id, type, useInt=useInt, **kwargs):
                return True
        return False
#         if re.match('OparamProbabilityHist', type, flags=re.I) or re.match('oph', type, flags=re.I):
#             self.oparamProbabilityHists.InitPlottable(id=id, useInt=useInt, **kwargs)
    
    def _InitPlottableClosure(self, longName, shortName, plottableContainer):
        def _InitPlottable(id, type, useInt=True, **kwargs):
            if re.match(longName, type, flags=re.I) or re.match(shortName, type, flags=re.I):
                plottableContainer.InitPlottable(id=id, useInt=useInt, **kwargs)
                return True
            else:
                return False
        return _InitPlottable
    
    def InitPlottableContainers(self):
        self.plottableContainers = {}
        self.initPlottables = {}
        for pC in self.plottableContainerTypes:
            self.__setattr__(CamelCase(pC.__name__), (pC(fPath=self.intermediatePath, sim=self)))
            self.plottableContainers[CamelCase(pC.__name__)] = self.__getattribute__(CamelCase(pC.__name__))
            singularName = Singular(pC.__name__)
            self.initPlottables[singularName] = self._InitPlottableClosure(longName=singularName, shortName=ShortenName(singularName), plottableContainer=self.__getattribute__(CamelCase(pC.__name__)))
#         print([id(initPlottable) for initPlottable in self.initPlottables.values()])
    
#     def transformData(self):
#         for plottable in self.plottables.items():
#             plottable.transformData()
#         for id, plottable in self.oparamProbabilityHists:
#             plottable.transformData()
#             
    
#         self.f = h5py.File(self.fPath)
#         self.params = self.f['/Parameters'].attrs
#         self.maxtime = self.params['maxTime']
#         self.writeinterval = self.params['writeInterval']
#         self.replicateKeys = list(self.f['/Simulations'].keys())
#         self.replicateCount = len(self.replicateKeys)
#         self.f.close()
#         
#         self.histogram = []
#         self.xBinCoordinates = []
#         self.yBinCoordinates = []
# #         self.p = []
#         for key in self.replicateKeys:
#             print('starting intermediate processing of replicate %07d' % int(key))
#             self.f = h5py.File(self.fPath)
#             simdata = self.f['/Simulations'][key]
#             sim = self.__class__.child(simdata, **self.sweepParams)
#             self.InitFPT()
#             self.InitOParamHist
#             self.
#             elif self.type=='replicateProbability':
#                 self.RcoordsHist(sim)
#                 self.PdfHist()
#             elif self.type=='fflux':
#                 self.GetFFluxOutput()
#             del sim
#             self.f.close()
#         self.Save()
    
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
        if hasattr(self, 'sweepParams'):
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
        