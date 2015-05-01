import h5py
import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

from ..datum.data import Data
from .oparam import OParam
from .oparamLinear import OParamLinear
from OrderParameters_pb2 import OrderParameters as OrderParametersBuf

class OParams(Data):
    hdf5RootGroup = 'OrderParameters'
    typeDict = {0:OParamLinear}
    
    def __init__(self, fPath):
        super().__init__(fPath)
        self.protoBuf = OrderParametersBuf()

#     def __getitem__(self, key):
#         return self.oparamMap[key]

    def Calc(self, oparamID, speciesVec):
        return self[oparamID].Calc(speciesVec)
    
#     def hasHDF5(self, hdf5File):
#         return self.hdf5RootGroup in hdf5File
    
    def _rffHDF5(self): 
        '''
        rff (read from file)
        '''
        for key,val in self.file[self.hdf5RootGroup].items():
            oparamBuf = self.protoBuf.order_parameters.add()
            self.map[val.attrs['ID']] = self.__class__.typeDict[val.attrs['Type']](oparamBuf=oparamBuf, hdf5OParamGroup=val)
