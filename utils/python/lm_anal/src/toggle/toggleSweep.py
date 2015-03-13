from ..main.sweep import Sweep
from toggleSim import ToggleSim

class toggleSweep(Sweep):
    child = ToggleSim
    def __init__(self, rootPath, unpickle=False):
        self.fname = rootPath.split('/')[-1]
        self.simsSweep = []
        self.sweepParams = {}
        for tup in os.walk(rootPath):
            for fname in tup[2]:
                if fname[-3:]=='.lm':
                    # the following regex captures things of the form (name)(float-val)_(name)(float-val)
                    sweepParamRe = re.search('([^\W\d_]+)([\d\.]+)_([^\W\d_]+)([\d\.]+)', tup[0])
#                     sweepParamTups = zip(sweepParamRe.groups()[::2],sweepParamRe.groups()[1::2])
                    sweepParamDict = {sweepParamRe.group(1): float(sweepParamRe.group(2)) + .00001,
                                      sweepParamRe.group(3): float(sweepParamRe.group(4)) + 1}
                    for key,val in sweepParamDict.iteritems():
                        tmpSweepParamsList = self.sweepParams.get(key, [])
                        if val not in tmpSweepParamsList:
                            self.sweepParams[key] = sorted(tmpSweepParamsList + [val])
                    fpath = os.path.join(tup[0], fname)
                    self.simsSweep.append(Biphasics(fpath, unpickle=unpickle, **sweepParamDict))
                    
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
        print 'graphing now...'
        rang = [[self.sweepParams[sweepParamNames[0]][0] - .00001, self.sweepParams[sweepParamNames[0]][-1] + (self.sweepParams[sweepParamNames[0]][-1] - self.sweepParams[sweepParamNames[0]][-2]) - .00001],
                [self.sweepParams[sweepParamNames[1]][0] - 1 , self.sweepParams[sweepParamNames[1]][-1] + (self.sweepParams[sweepParamNames[1]][-1] - self.sweepParams[sweepParamNames[1]][-2]) - 1]]
        bins = [len(self.sweepParams[sweepParamNames[0]]),
                len(self.sweepParams[sweepParamNames[1]])]
        hist = plt.hist2d(zip(*transitions)[0], zip(*transitions)[1], range=rang, bins=bins, cmap=cm.hot_r)
        plt.colorbar()
        xBinCenters = hist[1][:-1] + ((hist[1][1:] - hist[1][:-1])/2)
        yBinCenters = hist[2][:-1] + ((hist[2][1:] - hist[2][:-1])/2)
        normedHistArr = hist[0]/np.max(hist[0])
        print normedHistArr
        it = np.nditer([hist[0], normedHistArr], flags=['multi_index'])
        while not it.finished:
            print it[1], cm.hot_r(it[1]), AnnotationColor(*cm.hot_r(it[1]))
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
        print 'done' 
    
    def Save(self):
        for sims in self.simsSweep:
            sims.Save()