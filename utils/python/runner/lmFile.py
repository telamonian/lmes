#!/usr/bin/env python

from collections import namedtuple
import h5py
import numpy as np
import sys

# namedtuples (which are similar to C structs) that hold raw data used to initialize the Lattice Microbes input
Dependency = namedtuple('Dependency', ['reactionID','dependencies'])
DependencyMatrix = namedtuple('DependencyMatrix', ['matrix'])
InitialSpeciesCounts = namedtuple('InitialSpeciesCounts',['speciesCounts'])
InitialSpeciesCountsBackward = namedtuple('InitialSpeciesCountsBackward',['speciesCounts'])
OrderParameter = namedtuple('OrderParameter', ['id','type','speciesIDs','speciesCoefficients'])
ReactionRateConstant = namedtuple('ReactionRateConstant', ['reactionID','rateConstant'])
SimulationParameter = namedtuple('SimulationParameter', ['key', 'val'])
Tiling = namedtuple('Tiling', ['id','orderParameterID','type','edges'])

# namedtuples with default values
class Tiling(namedtuple('Tiling', ['id','orderParameterID','type','edges','isCurrentTiling'])):
    def __new__(cls, id,orderParameterID,type,edges,isCurrentTiling=False):
        return super(Tiling, cls).__new__(cls, id,orderParameterID,type,edges,isCurrentTiling)

# list of input tuple Types
inputTypes = [Dependency,DependencyMatrix,InitialSpeciesCounts,InitialSpeciesCountsBackward,OrderParameter,ReactionRateConstant,SimulationParameter,Tiling]
# function to import list of tuple Types
def GetTypes(types, name):
    thismodule = sys.modules[name]
    for t in types:
        setattr(thismodule, t.__name__, t)

class Input(object):
    def __init__(self, fPath, mode='a'):
        self.f = h5py.File(fPath, mode)      #h5py.File('.'.join((fname,'lm')), 'a')
    
    def Close(self):
        self.f.close()
        
    def Flush(self):
        self.f.flush()
    
    def AddTiling(self, tiling):
        '''
        currently an alias to SetTiling. maaaay cause problems
        '''
        self.SetTiling(tiling)
            
    def AddTilings(self, tilings, currentTilingID=None):
        '''
        currently an alias to SetTilings. maaaay cause problems
        '''
        self.SetTilings(tilings, currentTilingID=currentTilingID)

    def FixZerothOrderDependency(self, reactionID):
        self.f['/Model/Reaction/DependencyMatrix'][:,reactionID][...] = 1
        
    def FixZerothOrderDependencyAll(self):
        it = np.nditer(self.f['/Model/Reaction/ReactionTypes'], flags=['multi_index'])
        while not it.finished:
            if it[0] in (0,1000):   # if the reaction type indicates that this is one of the zeroth-order reactions, then...
                self.FixZerothOrderDependency(it.multi_index[0])
            it.iternext()
    
    def GetReactionRateConstants(self):
        return self.f['/Model/Reaction/ReactionRateConstants']
    
    def GetReplicates(self):
        replicateIDs = []
        shapes = np.zeros((len(self.f['/Simulations'].values()), 2), dtype=int)
        for i,(replicateID,data) in enumerate(self.f['/Simulations'].iteritems()):
            replicateIDs.append(replicateIDs)
            shapes[i,:] = data['SpeciesCounts'].shape
        colCount = shapes[0,1]
        rowCount = np.sum(shapes,0)[0]
        indices = zip(np.cumsum(shapes,0)[:-2,0], np.cumsum(shapes,0)[1:-1,0])
        speciesCounts = np.zeros((rowCount, colCount))
        speciesCountTimes = np.zeros((rowCount, 1))
        for i,data in enumerate(self.f['/Simulations'].itervals()):
            data['SpeciesCounts'].read_direct(speciesCounts, dest_sel=np.s_[indices[i][0]:indices[i][1], :colCount])
            data['SpeciesCountTimes'].read_direct(speciesCountTimes, dest_sel=np.s_[indices[i][0]:indices[i][1], :1])

    def SetInitialSpeciesCounts(self, iSCs):
        '''
        initializes the /Model/Reaction/initialSpeciesCounts dataset (set with this script in order to ensure consistency with the Backward version) and sets it
        iSCs: a list of InitialSpeciesCounts namedtuple
        '''
        if '/Model/Reaction/InitialSpeciesCounts' in self.f:
            del self.f['/Model/Reaction/InitialSpeciesCounts']
        if '/Model/Reaction' not in self.f:
            reactionModelGroup = self.f.create_group("/Model/Reaction")
        else:
            reactionModelGroup = self.f['/Model/Reaction']
        initialSpeciesCounts = reactionModelGroup.create_dataset("InitialSpeciesCounts", (len(iSCs.speciesCounts),), dtype=np.dtype('uint32'))
        initialSpeciesCounts[...] = iSCs.speciesCounts
        
    def SetInitialSpeciesCountsBackward(self, iSCBs):
        '''
        initializes the /Model/Reaction/initialSpeciesCountsBackward dataset (used to run the FFlux sampling in the Backward direction) and sets it
        iSCBs: a list of InitialSpeciesCountsBackward namedtuple
        '''
        if '/Model/Reaction/InitialSpeciesCountsBackward' in self.f:
            del self.f['/Model/Reaction/InitialSpeciesCountsBackward']
        if '/Model/Reaction' not in self.f:
            reactionModelGroup = self.f.create_group("/Model/Reaction")
        else:
            reactionModelGroup = self.f['/Model/Reaction']
        initialSpeciesCountsBackward = reactionModelGroup.create_dataset("InitialSpeciesCountsBackward", (len(iSCBs.speciesCounts),), dtype=np.dtype('uint32'))
        initialSpeciesCountsBackward[...] = iSCBs.speciesCounts
    
    def SetOrderParameter(self, op):
        '''
        initializes an order parameter according to id and then sets it
        op: an OrderParameter namedtuple
        '''
        if 'OrderParameters' in self.f.keys():
            if ('%07d' % op.id) in self.f['OrderParameters']:
                del self.f['OrderParameters/%07d' % op.id]
        opGroup = self.f.create_group('OrderParameters/%07d' % op.id)
        opGroup.attrs['Type'] = op.type
        opGroup.attrs['ID'] = op.id
        speciesIDs = opGroup.create_dataset("SpeciesIDs", (len(op.speciesIDs),), dtype=np.dtype('uint32'))
        speciesIDs[...] = op.speciesIDs
        speciesCoefficients = opGroup.create_dataset("SpeciesCoefficients", (len(op.speciesCoefficients),), dtype=np.dtype('d'))
        speciesCoefficients[...] = op.speciesCoefficients
            
    def SetOrderParameters(self, ops):
        '''
        initializes the OrderParameters group and then sets a list of OrderParameter
        ops: a list of OrderParameter namedtuple
        '''
        if 'OrderParameters' in self.f.keys():
            del self.f['OrderParameters']
        self.opsGroup = self.f.create_group('OrderParameters')
        for op in ops:
            self.SetOrderParameter(op)
    
    def SetReactionRateConstant(self, rRate):
        self.f['/Model/Reaction/ReactionRateConstants'][rRate.reactionID,0] = rRate.rateConstant
    
    def SetReactionRateConstants(self, rRates):
        '''
        sets at least some of the reaction rate constants in the associated table in the Reaction Model
        '''
        for rRate in rRates:
            self.SetReactionRateConstant(rRate)
    
    def SetSimulationParameter(self, simParam):
        '''
        sets a simulation parameter as an attribute on the Parameters group
        simParam: a SimulationParameter namedtuple
        '''
        print(simParam)
        self.f['Parameters'].attrs[simParam.key] = np.string_(str(simParam.val)+' ') # np.string_ conversion in place so that attr is stored as fixed length string
        
    def SetSimulationParameters(self, simParams):
        '''
        sets a list of SimulationParameter as attributes on the Parameters group
        simParams: list of SimulationParameter namedtuple
        '''
        for simParam in simParams:
            self.SetSimulationParameter(simParam)
            
    def SetTiling(self, tiling):
        '''
        initializes a tiling and then sets it.
        tiling: a Tiling namedtuple
        '''
        if 'Tilings' in self.f.keys():
            if ('%07d' % tiling.id) in self.f['Tilings']:
                del self.f['Tilings/%07d' % tiling.id]
        tilingGroup = self.f.create_group("Tilings/%07d" % tiling.id)
        tilingGroup.attrs['ID'] = tiling.id
        tilingGroup.attrs['OrderParameterID'] = tiling.orderParameterID
        tilingGroup.attrs['Type'] = tiling.type
        edges = tilingGroup.create_dataset("Edges", (len(tiling.edges),), dtype=np.dtype('d'))
        edges[...] = tiling.edges
        
        if tiling.isCurrentTiling:
            self.f['Tilings'].attrs['CurrentTilingID'] = tiling.id
            
    def SetTilings(self, tilings, currentTilingID=None):
        '''
        initializes the Tilings group and then adds a list of Tiling
        tilings: a list of Tiling namedtuple
        '''
        if 'Tilings' in self.f.keys():
            del self.f['Tilings']
        self.tilingsGroup = self.f.create_group('Tilings')
        for tiling in tilings:
            self.SetTiling(tiling)
        if currentTilingID==None:
            if len(self.tilingsGroup.keys()) > 0:
                self.tilingsGroup.attrs['CurrentTilingID'] = int(self.tilingsGroup.keys()[0])
        else:
            self.tilingsGroup.attrs['CurrentTilingID'] = currentTilingID
    
if __name__=="__main__":
    iSCs = InitialSpeciesCounts(speciesCounts=[4,16,1,0,0,0,0])
    iSCBs = InitialSpeciesCountsBackward(speciesCounts=[0,0,0,4,16,1,0])
    op = OrderParameter(type=0,
                        id=0,
                        speciesIDs=[0,1,2,3,4,5],
                        speciesCoefficients=[-1,-2,-2,1,2,2])
    simParams = [SimulationParameter(key='crossingsPerPhase',val='100'),
                 SimulationParameter(key='maxPhaseZeroTime',val='10000'),
                 SimulationParameter(key='maxSteps',val='100000'),
                 SimulationParameter(key='maxTime',val='Inf'),
                 SimulationParameter(key='maxWorkUnitSteps',val='10000'),
                 SimulationParameter(key='writeInterval',val='1e8')]
    tiling = Tiling(id=0,
                    orderParameterID=0,
                    type=0,
                    edges=np.linspace(-25,25,13))
    input = Input('biphasic_switch.lm')
    input.AddTilings(tilings=[tiling])
    input.SetInitialSpeciesCounts(iSCs=iSCs)
    input.SetInitialSpeciesCountsBackward(iSCBs=iSCBs)
    input.SetOrderParameters(ops=[op])
    input.SetSimulationParameters(simParams=simParams)
