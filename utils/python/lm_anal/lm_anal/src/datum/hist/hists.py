from lm_anal.src.datum import Data
from lm_anal.src.datum.hist import Hist

class Hists(Data):
    datumType = Hist
    
    def __init__(self):
        super().__init__()