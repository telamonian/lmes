from .reducer import Reducer

class Plottable(Reducer):
    '''
    base class for plottable objects.
    '''
    def __init__(self, sim):
        self.sim = sim
    
