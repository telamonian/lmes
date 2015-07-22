import numpy as np
import os,sys

from lm_anal.src.io.hdf5.fflux import FFluxBasinsIO, FFluxFinalsIO, FFluxOutputsIO, FFluxTrajectoriesIO
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
        self.intermediatePath = os.path.join(self.fDir, self.fName) + '.lmint'
        
        self.modIO = TimeIO(fPath=self.fPath)
        
#         if lmintOnly:
#             # we only have a .lmint file and no base .lm file
#             self._Load()
#         elif not self.Load():
#             self.Init()
            
        # logic of the following conditional:
        # if you want to unpickle an intermediate AND the raw data HAS NOT changed, then do so
        # otherwise if you want to unpickle an intermediate AND the raw data HAS changed, then work with the raw data
        # otherwise if you don't care about pickled anything, then work with the raw data
#         if unpickle==True:
#             try:
#                 if self.CheckMod():
#                     self.Load()
#                 else:
#                     self.Init()
#             except IOError:
#                 self.Init()
#         else:
#             self.Init()
    
#     def __str__(self):
#         outString = ''
#         for key,val in self.sweepParams.items():
#             outString+='%s: %s, ' % (key,val)
#         return outString[:-2]
#     
    def cook(self, recipeName, **kwargs):
        self.__getattribute__('%sRecipe' % recipeName)(**kwargs)
        
    def OParamHistsRecipe(self, tilingIDs, **kwargs):
        self.bfTrajsIO = BruteForceTrajectoriesIO(fPath=self.fPath  )
        self.oparamsIO = OParamsIO(fPath=self.fPath)
        self.tilingsIO = TilingsIO(fPath=self.fPath)
        
        self.oparams = OParams()
        self.opHists = OParamHists()
        self.specTrajs = SpeciesTrajectories()
        self.tilings = Tilings()
        
        self.bfTrajsIO.rff(container=self.specTrajs, full=True)
        self.oparamsIO.rff(container=self.oparams, full=True)
        self.tilingsIO.rff(container=self.tilings, full=True)
        
        tilings = [self.tilings[i] for i in tilingIDs]
        
        Transforms(src=self.specTrajs, dst=self.opHists, oparams=self.oparams, tilings=tilings)
        
    