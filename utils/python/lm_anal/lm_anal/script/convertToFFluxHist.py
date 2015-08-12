import h5py
from pathlib import Path
import numpy as np
import sys

class ConvertToFFluxHists(object):
    @property
    def srcRootPathStr(self):
        return str(self.srcRootPath)
    @property
    def dstRootPathStr(self):
        return str(self.dstRootPath)
    
    def __init__(self, srcRootPath, dstRootPath):
        self.srcRootPath = Path(srcRootPath)
        self.dstRootPath = Path(dstRootPath)
        
    def convert(self):
        for path in self.srcRootPath.rglob('*.lmint'):
            relPath = path.relative_to(self.srcRootPath)
            cfH = ConvertToFFluxHist(srcPath=self.srcRootPath / relPath, dstPath = self.dstRootPath / relPath)
            cfH.convert()
    
class ConvertToFFluxHist(object):
    @property
    def srcPathStr(self):
        return str(self.srcPath)
    @property
    def dstPathStr(self):
        return str(self.dstPath)
    
    def __init__(self, srcPath, dstPath):
        self.file = None
        self.srcPath = Path(srcPath)
        self.dstPath = Path(dstPath)
    
    def convert(self):
        self.wrapperSrcHdf5(self.getOldH)
        self.wrapperSrcHdf5(self.getOldEdges)
        
        self.addHalfOpenBinsToHist()
        
        self.wrapperDstHdf5(self.writeFFluxHist, 'w')
    
    def addHalfOpenBinsToHist(self):
        newH = np.zeros(np.array(self.h.shape)+2)
        newH[[np.s_[1:-1]]*len(self.h.shape)] = self.h
        self.h = newH
    
    def getOldH(self):
        histIter = self.file['Simulations'].values().__iter__()
        hHdf5 = next(histIter)['Histogram']
        self.h = np.zeros(hHdf5.shape)
        adder = self.h.copy()
        hHdf5.read_direct(self.h)
        for hist in histIter:
            hist['Histogram'].read_direct(adder)
            self.h+=adder
    
    def getOldEdges(self):
        edges = []
        hist = next(self.file['Simulations'].values().__iter__())
        for dim in ['XBinCoordinates', 'YBinCoordinates']:
            edges.append(hist[dim])
        self.h_edges = np.hstack(edges)
    
    def writeFFluxHist(self):
        opHistsGroup = self.file.create_group('OParamHists')
        sumGroup = opHistsGroup.create_group('Sum')
        sumGroup.attrs['h_threshold'] = 0.0
        sumGroup.attrs['h_weight'] = 1.0
        sumGroup.create_dataset("h", data=self.h)
        sumGroup.create_dataset("h_edges", data=self.h_edges)
        sumGroup.create_dataset("h_mask", data=np.ones(self.h.shape, dtype=bool))
        sumGroup.create_dataset("h_raw", data=self.h)
        
    def wrapperHdf5(self, path, func, mode='r', **kwargs):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the hdf5 file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant hdf5 file)), just run the function (with *args)
        '''
        if self.file==None:
            try:
                with h5py.File(path, mode) as self.file:
                    retVal = func(**kwargs)
            except OSError:
                self.file = None
                return False
            self.file = None
        else:
            retVal = func(**kwargs)
        return retVal
    
    def wrapperSrcHdf5(self, func, mode='r', **kwargs):
        self.wrapperHdf5(self.srcPathStr, func=func, mode=mode, **kwargs)
        
    def wrapperDstHdf5(self, func, mode='r', **kwargs):
        try:
            self.dstPath.parent.mkdir(parents=True)
        except FileExistsError:
            pass
        self.wrapperHdf5(self.dstPathStr, func=func, mode=mode, **kwargs)
        
if __name__=='__main__':
    srcRootPath = sys.argv[1]
    dstRootPath = sys.argv[2]
    
    convertToFFluxHists = ConvertToFFluxHists(srcRootPath=srcRootPath, dstRootPath=dstRootPath)
    convertToFFluxHists.convert()