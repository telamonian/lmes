class Replicate(object):
    def __init__(self, simdata):#, **kwargs):
        self.times = simdata['SpeciesCountTimes']
        self.counts = simdata['SpeciesCounts']
        self.oparam = None
        self.rcoords = None
#         self.sweepParams = kwargs
    
    @property
    def Oparam(self):
        return 
    
    @property
    def Rcoords(self):
    
    def __getstate__(self):
        """Return state values to be pickled."""
        return (self.oparam, self.rcoords, self.sweepParams, self.times)

    def __setstate__(self, state):
        """Restore state from the unpickled state values."""
        self.oparam, self.rcoords, self.sweepParams, self.times = state
        
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
            
        