from lm_anal.src.io.io import IO

class SFileIO(IO):
    '''
    base class for objects that perform io operations on sFiles
    '''

    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath

    def has(self):
        '''
        test if a file contains relevant data
        '''
        pass

    def keys(self):
        '''
        given a file and a kind of data that you're trying to retreive from it, this function returns a list of relevant data handles
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
        pass