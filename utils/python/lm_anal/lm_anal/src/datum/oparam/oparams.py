import os,sys

from lm_anal.src.datum import Data
from lm_anal.src.datum.oparam import OParam
from lm_anal.src.datum.oparam import OParamLinear
from lm_anal.src.helper import Tupify
from lm_anal.src.io.hdf5.oparam import OParamsIO

# thisScriptDir = os.path.dirname(os.path.realpath(__file__))
# sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf/lm/io'))
# sys.path.append(os.path.join(thisScriptDir, '../../../python_protobuf'))
from lm.io.OrderParameters_pb2 import OrderParameters as OParamsBuf

class OParams(Data):
    datumType = OParam
    Hdf5IOType = OParamsIO
    SFileType = None
    
    def __init__(self, protobuf=None, dataToTransform=None, fPath=None, transformKwargs=None):
        if protobuf==None:
            protobuf=OParamsBuf()
        super().__init__(protobuf=protobuf, dataToTransform=dataToTransform, fPath=fPath, transformKwargs=transformKwargs)

    def combine(self):
        vit = self.valIter()
        first,others = next(vit),list(vit)
        return first.combine(others)

    def combineByIDs(self, oparamIDs):
        oparams = self.sliceByKeys(oparamIDs)
        oit = oparams.valIter()
        first,others = next(oit),list(oit)
        try:
            return first.combine(others)
        except AttributeError:
            raise
    
    # def getByID(self, oparamIDs):
    #     oparamIDs = Tupify(oparamIDs)
    #     return [self[i] for i in oparamIDs]
    
    def initDatum(self, key, **kwargs):
        try:
            return self.map[key]
        except KeyError:
            subcon = self.protobuf.order_parameters.add()
            self.map[key] = self.datumType(subcon=subcon, **kwargs)
            return self.map[key]
