from src.datum.data import Data
from src.datum.hist.hists import Hist

class Tilings(Data):
    datumType = Hist
    
    def __init__(self):
        super().__init__()