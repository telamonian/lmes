import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

import h5py
from OrderParameters_pb2 import OrderParameters as OrderParametersBuf
from .oparam import OParam

class OParams(object):
    def __init__(self, fPath):
        self.fPath = fPath
        self.oparamsBuf = OrderParametersBuf()
        self.oparamMap = {}

    def __getitem__(self, key):
        return self.oparamMap[key]

    def rffHDF5(self): 
        '''
        rff (read from file)
        '''
        with h5py.File(self.fPath,'r') as simF:
            for key,val in simF['OrderParameters'].items():
                oparamBuf = self.oparamsBuf.order_parameters.add()
                self.oparamMap[val.attrs['ID']] = OParam(oparamBuf=oparamBuf, hdf5OParamGroup=val)