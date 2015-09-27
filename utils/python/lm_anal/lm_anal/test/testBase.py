from inspect import isclass, ismethod
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
from lm_anal.src.io.hdf5.trajectory import BruteForceTrajectoriesIO
from lm_anal.src.io.mod import TimeIO

from lm_anal.src.datum import Data
from lm_anal.src.datum.fflux import FFluxBasins, FFluxFinals, FFluxOutputs, FFluxTrajectories
from lm_anal.src.datum.hist import FFluxHists, OParamHists
from lm_anal.src.datum.parameter import SimulationParameters
from lm_anal.src.datum.oparam import OParams
from lm_anal.src.datum.tiling import Tilings
from lm_anal.src.datum.trajectory import SpeciesTrajectories, OParamTrajectories

from lm_anal.src.main import Sim

from lm_anal.src.transform import Transforms

dataTypeDict = {DataType.__name__:DataType for DataType in vars().values() if isclass(DataType) and issubclass(DataType, Data)}
# hdf5IOTypeDict = {Hdf5IOType.__name__:Hdf5IOType for Hdf5IOType in vars().values() if isclass(Hdf5IOType) and issubclass(Hdf5IOType, HDF5IO)}

__all__ = ['EagerTestBase', 'LazyTestBase', 'SimTestBase']

class BaseTestBase(object):
    dataTypeDict = dataTypeDict
#     hdf5IOTypeDict = hdf5IOTypeDict
    
    _dataTypeNames = None
    testFilePath = None
    
    @property
    def dataNames(self):
        return [CamelCaseLower(dataTypeName) for dataTypeName in self.dataTypeNames]
    
    @property
    def dataTypeNames(self):
        if self._dataTypeNames==None:
            return dataTypeDict.keys()
        else:
            return self._dataTypeNames
    
    @classmethod
    def simpleRun(cls):
        obj = cls()
        for testMethod in (obj.__getattribute__(methodName) for methodName in dir(obj) if ismethod(obj.__getattribute__(methodName)) and methodName[:4]=='test'):
            obj.setUp()
            testMethod()
            obj.tearDown()
    
    def setUp(self):
        print('setUp')
        pass
        self.cleanUpInt()
        
    def tearDown(self):
        print('tearDown')
        pass
        self.cleanUpInt()
    
    def loadData(self, full=False, **kwargs):
        pass
    
    def loadDataEagerly(self, full=False):
        self.loadData(full=full)
        for dataName in self.dataNames:
            self.__getattribute__(dataName).map
    
    def cleanUpInt(self):
        # make sure all of the .lmint/.mod stuff is cleaned up
        try:
            os.remove(str(self.testFilePath.with_suffix('.mod')))
        except FileNotFoundError:
            pass
        try:
            os.remove(str(self.testFilePath.with_suffix('.lmint')))
        except FileNotFoundError:
            pass

    def assertArraysEqual(self, arr1, arr2):
        try:
            testBool = (arr1==arr2).all()
        except AttributeError:
            testBool = False
        self.assertTrue(testBool, msg='not equal: %s\n%s' % (arr1.tolist(), arr2.tolist()))
        

class EagerTestBase(BaseTestBase):
    def loadData(self, full=False, fileType='hdf5', **kwargs):
        for dataTypeName in self.dataTypeNames:
            DataType = self.dataTypeDict[dataTypeName]
            data = DataType()
            self.__setattr__(CamelCaseLower(dataTypeName), data)
            
            if fileType=='hdf5':
                IOType = DataType.Hdf5IOType
            elif fileType=='sfile':
                IOType = DataType.SFileIOType
            io = IOType(fPath=self.testFilePath)
            self.__setattr__(CamelCaseLower, io)
            io.rff(container=data, full=full)
        self.doTransforms()
        
    def doTransforms(self):
        pass
#         tilingIDs = [1,2]
#         
#         Transforms(srcs={self.specTraj, self.ffluxOuts}, dsts=self.ffluxHists, oparams=self.oparams, simulationParameters=self.simParams, tilings=self.tilings, tilingIDs=tilingIDs)
        
class LazyTestBase(BaseTestBase):
    def loadData(self, full=False, **kwargs):
        for dataTypeName in self.dataTypeNames:
            DataType = self.dataTypeDict[dataTypeName]
            data = DataType(fPath=self.testFilePath)
            self.__setattr__(CamelCaseLower(dataTypeName), data)
        
        for dataName,transformKwargs in self.getTransformKwargsDict().items():
            self.__getattribute__(dataName).__setattr__('transformKwargs', transformKwargs)
        
    def getTransformKwargsDict(self):
        pass
        
#         transformKwargs = {'oparams':self.oparams, 'simulationParameters':self.simParams, 'tilings':self.tilings, 'tilingIDs':(1,2)}
#         
#         self.ffluxHists = FFluxHists(dataToTransform={self.specTrajs, self.ffluxOuts}, fPath=str(testFilePath), transformKwargs=transformKwargs)

class SimTestBase(BaseTestBase):
    def loadData(self, full=False, **kwargs):
        self.sim = Sim(fPath=self.testFilePath)
        for dataName,data in self.sim.items():
            self.__setattr__(dataName, data)
        
        for dataName,transformKwargs in self.getTransformKwargsDict().items():
            self.__getattribute__(dataName).__setattr__('transformKwargs', transformKwargs)
    
    def getTransformKwargsDict(self):
        return dict()