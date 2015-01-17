class Biphasic(Sim):
    basindict = {'A':0,'B':1}

    def Basin(self, framei):
        if self.oparam[framei] < -25:
            return 'A'
        elif self.oparam[framei] > 25:
            return 'B'
        else:
            return False

    def Pdf(self, limitToTravelTime=True):
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
