from .transform import Transform

class Plottable(Transform):
    '''
    base class for plottable objects.
    '''
    def __init__(self, sim):
        self.sim = sim
    
