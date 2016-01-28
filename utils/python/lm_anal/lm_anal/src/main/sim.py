from collections import OrderedDict
from inspect import isclass
from itertools import chain
import numpy as np
import os,sys
from pathlib import Path

from lm_anal.src.helper import CamelCaseLower
from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.io.hdf5.fflux import FFluxBasinsIO, FFluxFinalsIO, FFluxOutputsIO, FFluxTrajectoriesIO
from lm_anal.src.io.hdf5.hist import FFluxHistsIO, OParamHistsIO
from lm_anal.src.io.hdf5.parameter import SimulationParametersIO
from lm_anal.src.io.hdf5.oparam import OParamsIO
from lm_anal.src.io.hdf5.tiling import TilingsIO
from lm_anal.src.io.hdf5.trajectory import SpeciesTrajectoriesIO
from lm_anal.src.io.mod import TimeIO
from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxBasins, FFluxFinals, FFluxOutputs, FFluxTrajectories
from lm_anal.src.datum.fpt import OParamFPTs, SpeciesFPTs
from lm_anal.src.datum.hist import FFluxHists, OParamHists
from lm_anal.src.datum.model import ReactionModels
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.probability import InterfaceFluxes, TransitionProbabilities
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import OParamTrajectories
from lm_anal.src.datum.trajectory import SpeciesTrajectories
from lm_anal.src.transform import Transforms

_inputDataTypes = [OParams, ReactionModels, SimulationParameters, Tilings]
_dataTypes = [DataType for DataType in vars().values() if isclass(DataType) and issubclass(DataType, Data) and not DataType in _inputDataTypes]
# hdf5IOTypes = [hdf5IOType for hdf5IOType in vars().values() if isclass(hdf5IOType) and issubclass(hdf5IOType, HDF5IO)]

class SimMetaclass(type):
    def __new__(cls, clsname, bases, dct):
        return super(SimMetaclass, cls).__new__(cls, clsname, bases, dct)

class Sim(object):
    inputDataTypes = _inputDataTypes
    dataTypes = _dataTypes
        
    @property
    def fPathStr(self):
        return str(self.fPath)

# initializers
    def __init__(self, fPath, lazyLoad=True, lmintOnly=False, name=None, **kwargs):
        # path setting stuff
        self.fPath = Path(fPath)
#         self.fDir, self.fNameFull = os.path.split(self.fPath)
#         self.fName, self.fNameSuffix = self.fNameFull.split('.')[:2]
#         self.intermediatePath = os.path.join(self.fDir, '.' + self.fName) + '.lmint'
        
        self.modIO = TimeIO(fPath=self.fPathStr)
        self.initInputData(lazyLoad=lazyLoad)
        self.initData(lazyLoad=lazyLoad)
    
    def initData(self, lazyLoad=True):
        self.dataDict = OrderedDict()
        for DataType in self.dataTypes:
            dataName = CamelCaseLower(DataType.__name__)
            self.__setattr__(dataName, DataType(fPath=self.fPath, dataToTransformDict=self.inputDataDict, lazyLoad=lazyLoad))
            self.dataDict[dataName] = self.__getattribute__(dataName)
            
        self.ffluxHists.dataToTransform = self.ffluxOutputs
        self.interfaceFluxes.dataToTransform = self.ffluxOutputs
        self.oparamFPTs.dataToTransform = self.ffluxOutputs
        self.oparamHists.dataToTransform = self.speciesTrajectories
        self.oparamTrajectories.dataToTransform = self.speciesTrajectories
        self.transitionProbabilities.dataToTransform = self.ffluxOutputs
    
    def initInputData(self, lazyLoad=True):
        self.inputDataDict = OrderedDict()
        for InputDataType in self.inputDataTypes:
            inputDataName = CamelCaseLower(InputDataType.__name__)
            self.__setattr__(inputDataName, InputDataType(fPath=self.fPath, lazyLoad=lazyLoad))
            self.inputDataDict[inputDataName] = self.__getattribute__(inputDataName)
    
# accessors
    def items(self):
        return (item for item in chain(self.inputDataDict.items(), self.dataDict.items()))

# mutators
    def clear(self):
        for d in (self.inputDataDict, self.dataDict):
            for key in d():
                try: 
                    self.__delattr__(key)
                except AttributeError:
                    pass
                toDel = d.pop(key)
                del toDel
    
#     def gen(self, recipeName, freshenInt=True, readInt=True, writeInt=True, **kwargs):
#         # if we're told not to bother reading the .lmint, or if the restore "fails", use the relevant recipe to generate the data
#         if not readInt or not self.restore(recipeName, freshenInt, **kwargs):
#             self.__getattribute__('%sRecipe' % recipeName)(**kwargs)
#             # if we're writing out .lmints, do it now
#             if writeInt:
#                 self.save(recipeName, **kwargs)
#         
#     def restore(self, recipeName, freshenInt, **kwargs):
#         # if intermediate file does not exist, gen the data
#         if not os.path.isfile(self.intermediatePath):
#             return False
#         # if freshenInt is False, restroe from existing .lmint no matter what. Otherwise, restore from .lmint if the mod file says it's fresh
#         if not freshenInt or self.modIO.checkMod():
#             self.__getattribute__('%sRestore' % recipeName)(**kwargs)
#             return True
#         else:
#             return False
#     
#     def save(self, recipeName, **kwargs):
#         self.__getattribute__('%sSave' % recipeName)(**kwargs)
#         self.modIO.saveMod()
#         
#     def OParamHistsRecipe(self, tilingIDs, **kwargs):
#         bfTrajsIO = SpeciesTrajectoriesIO(fPath=self.fPath)
#         oparamsIO = OParamsIO(fPath=self.fPath)
#         tilingsIO = TilingsIO(fPath=self.fPath)
#         
#         self.oparams = OParams()
#         self.opHists = OParamHists()
#         self.specTrajs = SpeciesTrajectories()
#         self.tilings = Tilings()
#         
#         bfTrajsIO.rff(container=self.specTrajs, full=True)
#         oparamsIO.rff(container=self.oparams, full=True)
#         tilingsIO.rff(container=self.tilings, full=True)
#         
#         tilings = [self.tilings[i] for i in tilingIDs]
#         
#         Transforms(src=self.specTrajs, dst=self.opHists, oparams=self.oparams, tilings=tilings)
#         
#     def OParamHistsRestore(self, **kwargs):
#         oparamHistsIO = OParamHistsIO(fPath=self.intermediatePath)
#         
#         self.opHists = OParamHists()
#         
#         oparamHistsIO.rff(container=self.opHists, full=True)
#         
#     def OParamHistsSave(self, **kwargs):
#         opHistsIO = OParamHistsIO(fPath=self.intermediatePath)
#         
#         opHistsIO.wtf(container=self.opHists)