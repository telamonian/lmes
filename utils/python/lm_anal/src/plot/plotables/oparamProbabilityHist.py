import os,sys
thisScriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisScriptDir, '../../python_protobuf/lm/io'))

import h5py
from Hist_pb2 import Hist as HistBuf



class OParamProbabilityHist(object):
    def __init__(self, oparam, tiling):
        self.oparam = oparam
        self.tiling = tiling
        