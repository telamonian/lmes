import os

from lm_anal.src.io.mod.modIO import ModIO

class TimeIO(ModIO):
    def checkMod(self):
        '''
        check the mod time file (written with SaveMod()) corresponding to file. 
        If the simulation file has changed since the mod time file was written, return False. Otherwise, return True
        '''
        self.rff()
        return self.modTime==os.path.getmtime(self.fPath)
    
    def _rff(self, full, keys):
        '''
        internal rff (read from file) for time data stored in mod files
        '''
        self.modTime = float(self.file.readline())
    
    def saveMod(self):
        '''
        write out a mod time file corresponding to file.
        the format will be a single line with the time the simulation file was last modified
        to remove all .mod files from a dir tree in bash, use: rm */.[^.]*.mod
        '''
        self.modTime = os.path.getmtime(self.fPath)
        self.wtf()
        
    def _wtf(self, full, keys):
        self.file.write('%.1f' % self.modTime)