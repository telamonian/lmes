from ..io.fileHDF5 import FileHDF5
from .oparamProbabilityHist import OParamProbabilityHist

class OParamProbabilityHists(FileHDF5):
    hdf5RootPath = '/Hist/OParam'
    subType = OParamProbabilityHist
    
    def _rffHDF5(self, full, keys):
        '''
        rff (read from file) method for HDF5 lmint files
        '''
        if keys==None:
            keys = self.file[self.hdf5RootPath].keys()
        
        for key in keys:
            val = self.file[os.path.join(self.hdf5RootPath, key)]
            self.map[int(key)] = self.subType(hdf5Group=val)
            
    def _wtfHDF5(self):
        '''
        wtf (write to file) method for HDF5 lmint files
        '''
        if self.hdf5RootPath in self.file:
            del self.file[self.hdf5RootPath]
        hdf5RootGroup = self.file.create_group(hdf5RootPath)
        for key,opprobhist in self:
            opprobhist._wtfHDF5(hdf5RootGroup=hdf5RootGroup)