import h5py

# namedtuples (which are similar to C structs) that hold raw data used to initialize the hdf5 .lm file
InitialSpeciesCounts = namedtuple('InitialSpeciesCounts',['speciesCounts'])
InitialSpeciesCountsBackward = namedtuple('InitialSpeciesCountsBackward',['speciesCounts'])
OrderParameter = namedtuple('OrderParameter', ['id','type','speciesIDs','speciesCoefficients'])
ReactionRateConstant = namedtuple('ReactionRateConstant', ['reactionID','rateConstant'])
SimulationParameter = namedtuple('SimulationParameter', ['key', 'val'])
Tiling = namedtuple('Tiling', ['ID','orderParameterID','Type','edges'])

class Lm(object):
    def __init__(self, fname):
        self.f = h5py.File(fname, 'a')      #h5py.File('.'.join((fname,'lm')), 'a')
    
    def Close(self):
        self.f.close()
    
    def AddTiling(self, tiling):
        '''
        initializes a tiling and then sets it. this method is prefixed with Add instead of Set since it sets the id of the new tiling based on the length of the existing tiling group
        tiling: a Tiling namedtuple
        '''
        tilingGroup = self.f.create_group("Tilings/%07d" % len(self.tilingsContainingGroup.keys()))
        tilingGroup.attrs['ID'] = tiling.ID
        tilingGroup.attrs['OrderParameterID'] = tiling.orderParameterID
        tilingGroup.attrs['Type'] = tiling.Type
        edges = tilingGroup.create_dataset("Edges", (len(tiling.edges),), dtype=np.dtype('d'))
        edges[...] = tiling.edges
            
    def AddTilings(self, tilings):
        '''
        initializes the Tilings group and then adds a list of Tiling
        tilings: a list of Tiling namedtuple
        '''
        if 'Tilings' in self.f.keys():
            del self.f['Tilings']
        self.tilingsContainingGroup = self.f.create_group('Tilings')
        for tiling in tilings:
            self.AddTiling(tiling)

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
        opGroup = self.f.create_group("OrderParameters/%07d" % op.id)
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
        self.opsContainingGroup = self.f.create_group('OrderParameters')
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
        self.f['Parameters'].attrs[simParam.key] = np.string_(str(simParam.val)+' ') # np.string_ conversion in place so that attr is stored as fixed length string
        
    def SetSimulationParameters(self, simParams):
        '''
        sets a list of SimulationParameter as attributes on the Parameters group
        simParams: list of SimulationParameter namedtuple
        '''
        for simParam in simParams:
            self.SetSimulationParameter(simParam)