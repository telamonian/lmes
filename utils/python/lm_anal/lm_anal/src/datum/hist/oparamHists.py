from lm_anal.src.datum.hist.hists import Hists
from lm_anal.src.datum.hist.oparamHist import OParamHist
from lm_anal.src.io.hdf5.hist import OParamHistsIO

__all__ = ['OParamHists']

class OParamHists(Hists):
    datumType = OParamHist
    hdf5IOType = OParamHistsIO
    sfileIOType = None