import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

from ..datum.data import Data
from .oparam import OParam
from .oparamLinear import OParamLinear
from OrderParameters_pb2 import OrderParameters as OrderParametersBuf

class OParams(Data):
    hdf5RootPath = 'OrderParameters'
    typeDict = {0:OParamLinear}
    
    def __init__(self, fPath):
        super().__init__(fPath)
        self.protobuf = OrderParametersBuf()

    def Calc(self, oparamID, speciesVec):
        return self[oparamID].Calc(speciesVec)
    
    def _rff(self, full, keys):
        '''
        rff (read from file)
        '''
        for key,val in self.file[self.hdf5RootPath].items():
            oparamBuf = self.protobuf.order_parameters.add()
            self.map[val.attrs['ID']] = self.__class__.typeDict[val.attrs['Type']](oparamBuf=oparamBuf, hdf5OParamGroup=val)
