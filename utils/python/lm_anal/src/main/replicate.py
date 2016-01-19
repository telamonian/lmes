class Replicate(object):
    def __init__(self, simdata):#, **kwargs):
        self.times = simdata['SpeciesCountTimes']
        self.counts = simdata['SpeciesCounts']
#         self.sweepParams = kwargs
    
    @property
    def oparam(self):
        return
    
    def __getstate__(self):
        """Return state values to be pickled."""
        return (self.oparam, self.rcoords, self.sweepParams, self.times)

    def __setstate__(self, state):
        """Restore state from the unpickled state values."""
        self.oparam, self.rcoords, self.sweepParams, self.times = state
    
    
    def __init__(self, fPath, unpickle=False, **kwargs):
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.modTimePath = os.path.join(self.fDir, '.' + self.fName) + '.mod'
        self.picklePath = os.path.join(self.fDir, self.fName) + '.lmint'
        self.sweepParams = kwargs
        self.oparam = None
        self.rcoords = None
        
        # logic of the following conditional:
        # if you want to unpickle an intermediate AND the raw data HAS NOT changed, then do so
        # otherwise if you want to unpickle an intermediate AND the raw data HAS changed, then work with the raw data
        # otherwise if you don't care about pickled anything, then work with the raw data
        if unpickle==True:
            try:
                if self.CheckMod():
                    self.Load()
                else:
                    self.Init()
            except IOError:
                self.Init()
        else:
            self.Init()
    
    def __str__(self):
        outString = '\n'
        for key,val in self.sweepParams.items():
            outString+='%s: %s, ' % (key,val)
        return outString[:-2]
    
    def Init(self):
        self.f = h5py.File(self.fPath)
        self.params = self.f['/Parameters'].attrs
        self.maxtime = self.params['maxTime']
        self.writeinterval = self.params['writeInterval']
        self.sims = []
        for simdata in self.f['/Simulations'].values():
            self.sims.append(self.__class__.child(simdata, **self.sweepParams))
#         self.Pdf()
#         self.Rcoords()
    
    def CheckMod(self):
        '''
        check the mod time file (written with SaveMod()) corresponding to this simulation file. 
        If the simulation file has changed since the mod time file was written, return False. Otherwise, return True
        '''
        with open(self.modTimePath, 'r') as modF:
            return float(modF.readline())==os.path.getmtime(self.fPath)
    
    def SaveMod(self):
        '''
        write out a mod time file corresponding to this simulation file.
        the format will be a single line with the time the simulation file was last modified
        to remove all .mod files from a dir tree in bash, use: rm */.[^.]*.mod
        '''
        with open(self.modTimePath, 'w') as modF:
            modF.write('%f' % os.path.getmtime(self.fPath))
    
    def Load(self):
        with open(self.picklePath, 'rb') as pickF:
            self.sims = pickle.load(pickF)
        
    def Save(self):
        self.SaveMod()
        with open(self.picklePath, 'wb') as pickF:
            pickle.dump(self.sims, pickF)
    
    
    
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
        if self.pdf==None:
            self._Pdf()
    
    def Progress(self, maxt):
        return self.times[-1]/float(maxt)
    
    def Rcoords(self):
        if self.rcoords==None:
            self._Rcoords()
            
        