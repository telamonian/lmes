#!/usr/local/bin/python
import h5py, sys, shutil, os
import numpy as np
# import matplotlib
# matplotlib.use('Agg')
# import matplotlib.pyplot as plt
# import matplotlib.cm as cm
# from matplotlib.colors import LogNorm
# 
# from fit import Fit

class AlterData(object):
    def __init__(self, path, sourceFname, k):
        self.k = k
        self.path = path
        self.sourceFname = sourceFname
        self.fname = '.'.join(sourceFname.split('.')[:-1]) + "_k%3.2f"%self.k + '.lm'
        shutil.copy2(os.path.join(path,sourceFname), os.path.join(path,self.fname))
        
        self.f = h5py.File(os.path.join(path,self.fname))
        self.params = self.f['/Parameters'].attrs
        self.params.modify('writeInterval', '1e2')
        
        self.initialSpeciesCounts = self.f['/Model/Reaction/InitialSpeciesCounts']
        self.bBasinOparam = self.k*37.0
        self.bCount = np.floor((-1.0 + np.sqrt(1 + 8*self.bBasinOparam))*(1.0/4.0))
        self.bDimerCount = np.floor(np.power((-1.0 + np.sqrt(1 + 8*self.bBasinOparam))*(1.0/4.0), 2))
        self.initialSpeciesCounts[:] = [0,0,0,self.bCount,self.bDimerCount,0,1]
        
        self.reactionRateConstants = self.f['/Model/Reaction/ReactionRateConstants']
        self.reactionRateConstants[:,0] = [5,5,5,1,self.k,self.k,.25]*2 
        self.f.flush()
        self.f.close()
        
if __name__=="__main__":
    path = sys.argv[1]
    sourceFname = sys.argv[2]
    #for i in range(1,11):
    for i in (1.0,2.0):
        AlterData(path, sourceFname, i)
