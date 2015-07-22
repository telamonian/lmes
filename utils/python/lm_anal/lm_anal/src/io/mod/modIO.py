import os

from lm_anal.src.io.io import IO

class ModIO(IO):
    '''
    base class for objects that perform io operations on sFiles
    '''

    def __init__(self, fPath):
        self.file = None
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.modFPath = os.path.join(self.fDir, '.' + self.fName) + '.mod'

    def checkMod(self):
        '''
        check the mod time file (written with SaveMod()) corresponding to file. 
        If the simulation file has changed since the mod time file was written, return False. Otherwise, return True
        '''
        pass
    
    def _has(self):
        '''
        internal has method for a mod file
        '''
        pass
        
    def has(self):
        '''
        test if a mod file has data relevant to this particular ModIO object
        '''
        return self.wrapperMod(self._has)

    def _keys(self):
        '''
        intenral keys method for data stored in mod files
        '''
        pass

    def keys(self):
        return self.wrapperHDF5(self._keys)

    def _rff(self, full, keys):
        '''
        internal rff (read from file) for data stored in mod files
        '''
        pass
    
    def rff(self, full=False, keys=None, **kwargs):
        '''
        rff (read from file) for mod files
        '''
        return self.wrapperMod(self._rff, full=full, keys=keys, **kwargs)

    def saveMod(self):
        '''
        write out a mod time file.
        to remove all .mod files from a dir tree in bash, use: rm */.[^.]*.mod
        '''
        pass

    def _wtf(self, full, keys):
        '''
        internal generic wtf (read from file) for data stored in mod files
        '''
        pass
        
    def wtf(self, full=True, keys=None, **kwargs):
        '''
        wtf (write to file) for mod files
        '''
        self.wrapperMod(self._wtf, full=full, mode='w', keys=keys, **kwargs)

    def wrapperMod(self, func, mode='r', **kwargs):
        '''
        if self.file==None, run the function (with *args) inside a 'with' block that assigns the file object to self.file, then resets self.file to None
        else (self.file already contains something (hopefully the relevant  file)), just run the function (with *args)
        '''
        if self.file==None:
            try:
                with open(self.modFPath, mode) as self.file:
                    retVal = func(**kwargs)
            except OSError:
                self.file = None
                return False
            self.file = None
        else:
            retVal = func(**kwargs)
        return retVal