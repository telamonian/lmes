class Time(object):
    def CheckMod(self):
        '''
        check the mod time file (written with SaveMod()) corresponding to file. 
        If the simulation file has changed since the mod time file was written, return False. Otherwise, return True
        '''
        with open(self.modTimePath, 'r') as modF:
            return float(modF.readline())==os.path.getmtime(self.fPath)
    
    def SaveMod(self):
        '''
        write out a mod time file corresponding to file.
        the format will be a single line with the time the simulation file was last modified
        to remove all .mod files from a dir tree in bash, use: rm */.[^.]*.mod
        '''
        with open(self.modTimePath, 'w') as modF:
            modF.write('%f' % os.path.getmtime(self.fPath))
    
