from lm_anal.src.datum.data import Data
from lm_anal.src.datum.hist.hists import Hist

class Tilings(Data):
    datumType = Hist
    
    def __init__(self):
        super().__init__()