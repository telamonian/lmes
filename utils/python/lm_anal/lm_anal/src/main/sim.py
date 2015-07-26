import numpy as np
import os,sys

from lm_anal.src.io.hdf5.fflux import FFluxBasinsIO, FFluxFinalsIO, FFluxOutputsIO, FFluxTrajectoriesIO
from lm_anal.src.io.hdf5.hist import OParamHistsIO
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO
from lm_anal.src.io.mod import TimeIO
from lm_anal.src.datum.fflux import FFluxBasins, FFluxFinals, FFluxOutputs, FFluxTrajectories
from lm_anal.src.datum.hist import OParamHists
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories
from lm_anal.src.transform import Transforms

class SimMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sim(object):
    def __init__(self, fPath, lmintOnly=False, **kwargs):
        # path setting stuff
        self.fPath = fPath
        self.fDir, self.fNameFull = os.path.split(self.fPath)
        self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
        self.intermediatePath = os.path.join(self.fDir, '.' + self.fName) + '.lmint'
        
        self.modIO = TimeIO(fPath=self.fPath)
           
    def clear(self):
        for attrName in ('oparams, opHists, specTrajs, tilings'):
            try: 
                self.__delattr__(attrName)
            except AttributeError:
                pass
    
    def gen(self, recipeName, freshenInt=True, readInt=True, writeInt=True, **kwargs):
        # if we're told not to bother reading the .lmint, or if the restore "fails", use the relevant recipe to generate the data
        if not readInt or not self.restore(recipeName, freshenInt, **kwargs):
            self.__getattribute__('%sRecipe' % recipeName)(**kwargs)
            # if we're writing out .lmints, do it now
            if writeInt:
                self.save(recipeName, **kwargs)
        
    def restore(self, recipeName, freshenInt, **kwargs):
        # if intermediate file does not exist, gen the data
        if not os.path.isfile(self.intermediatePath):
            return False
        # if freshenInt is False, restroe from existing .lmint no matter what. Otherwise, restore from .lmint if the mod file says it's fresh
        if not freshenInt or self.modIO.checkMod():
            self.__getattribute__('%sRestore' % recipeName)(**kwargs)
            return True
        else:
            return False
    
    def save(self, recipeName, **kwargs):
        self.__getattribute__('%sSave' % recipeName)(**kwargs)
        self.modIO.saveMod()
        
    def OParamHistsRecipe(self, tilingIDs, **kwargs):
        bfTrajsIO = BruteForceTrajectoriesIO(fPath=self.fPath)
        oparamsIO = OParamsIO(fPath=self.fPath)
        tilingsIO = TilingsIO(fPath=self.fPath)
        
        self.oparams = OParams()
        self.opHists = OParamHists()
        self.specTrajs = SpeciesTrajectories()
        self.tilings = Tilings()
        
        bfTrajsIO.rff(container=self.specTrajs, full=True)
        oparamsIO.rff(container=self.oparams, full=True)
        tilingsIO.rff(container=self.tilings, full=True)
        
        tilings = [self.tilings[i] for i in tilingIDs]
        
        Transforms(src=self.specTrajs, dst=self.opHists, oparams=self.oparams, tilings=tilings)
        
    def OParamHistsRestore(self, **kwargs):
        oparamHistsIO = OParamHistsIO(fPath=self.intermediatePath)
        
        self.opHists = OParamHists()
        
        oparamHistsIO.rff(container=self.opHists, full=True)
        
    def OParamHistsSave(self, **kwargs):
        opHistsIO = OParamHistsIO(fPath=self.intermediatePath)
        
        opHistsIO.wtf(container=self.opHists)