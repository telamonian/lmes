from lm_anal.src.datum.hist import Hists
from lm_anal.src.datum.hist import OParamHist

class OParamHists(Hists):
    datumType = OParamHist
    
    def __init__(self):
        super().__init__()