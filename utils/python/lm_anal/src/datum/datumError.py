class dimensioningError(Exception):
    '''
    gets called when there's some problem with the rank and/or dims of an arrayDatum
    '''
    def __init__(self, rank, dims):
        self.rank = rank
        self.dims = dims

    def __str__(self):
        s = 'rank: %d incompatible with dims: %s' % (self.rank, self.dims)
        return s