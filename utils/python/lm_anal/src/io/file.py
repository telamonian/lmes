class File(object):
    '''
    base class for all other objects that map closely to on-drive files
    '''
    hdf5RootPath = None

    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath
        self.map = {}

    def __delitem__(self, key):
        del self.map[key]

    def __getitem__(self, key):
        return self.map[key]

    def __iter__(self):
        return self.map.items().__iter__()

    def has(self):
        '''
        test if a file contains relevant data
        '''
        pass

    def rff(self, full=False, keys=None, **kwargs):
        '''
        rff (read from file)
        '''
        pass

    def sff(self, keys=None):
        '''
        sff (stream from file)
        '''
        pass

    def wtf(self, keys=None):
        '''
        wtf (write to file)
        '''