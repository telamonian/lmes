from matplotlib import _cntr as cntr
from matplotlib import cm
from matplotlib import gridspec
from matplotlib import pyplot as __plt
import numpy as np

_plt = __plt
_fig = _plt.figure(1)
_ax0 = _plt.subplot(gridspec.GridSpec(1,1,wspace=0.15,hspace=0.07)[0,0])

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

def AutoscaleAxes():
    _ax0.relim()
    _ax0.autoscale_view(True,True,True)

def SetBackground(background, smooth=True):
    if smooth:
        _plt.imshow(background, cmap=_plt.cm.gray, origin='lower')
    else:
        _plt.pcolormesh(background, cmap=_plt.cm.gray)

def SavePlot(outfile):
    _fig.set_size_inches(12,12)
    _plt.savefig(outfile, bbox_inches='tight')

def ShowPlot():
    _plt.show()

class Plot(object):
    @property
    def ax0(self):
        return _ax0
    
    @property
    def IsClosedLike(self):
        return self.IsClosed() or self.Dist(self.points[0], self.points[-1]) < self.avgDist + 2*self.stdDist
    
    @property
    def fig(self):
        return _fig
    
    @property
    def plt(self):
        return _plt
    
    def __init__(self):
        pass
        
    def plotTrajectory(self, x, y=None, z=None, time=None):
        if y!=None:
            self.ax0.plot(x, y)