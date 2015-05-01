from ..main.replicate import Replicate
from . import hdf5
from . import mod

class simFileBase(object):
    def __init__(self, fPath):
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.modTimePath = os.path.join(self.fDir, '.' + self.fName) + '.mod'
        self.intPath = os.path.join(self.fDir, self.fName) + '.lmint'
    
    def CheckMod(self):
        pass
    def Load(self):
        pass
    def LoadInt(self):
        pass
    def Save(self):
        pass
    def SaveInt(self):
        pass
    def SaveMod(self):
        pass

def simFileFactory(name='simFile', lmBase=hdf5.Lm, lmIntBase=hdf5.LmInt, modBase=mod.Time, dict={}):
    bases = [lmBase, lmIntBase, modBase, simFileBase]
    return type(name=name, bases=bases, dict=dict)
