runnerPath = '/Users/tel/git/lm/utils/python/runner'
import sys
if runnerPath not in sys.path:
    sys.path.append(runnerPath)

from lmFile import *
from sweep import SweepTup

# "master" functions. For the most part, these are the functions endpoint scripts should be calling
def GenBFDefaultInputTup(basin):
    '''
    generate some input tups with some brute force-specific default values
    '''
    simParamDict =  {'maxCrossingsZero': 1e5,
                     'maxCrossingsN':    1e5,
                     'maxSteps':         1e15,
                     'maxWorkUnitSteps': 1e8}

    return GenDefaultInputTup(basin=basin, simParamDict=simParamDict)

def GenFFluxDefaultInputTup():
    '''
    generate some input tups with some forward flux-specific default values
    '''
    simParamDict =  {'maxSteps':         1e15,
                     'maxWorkUnitSteps': 1e15}

    return GenDefaultInputTup(basin='a', simParamDict=simParamDict)

def GenDefaultInputTup(basin, simParamDict):
    ops = GenOParamDefaultInputTup()
    sCounts = GenInitialSpeciesCountDefaultInputTup(basin)
    simParams = [SimulationParameter(key=key,val=str(int(val))) for key,val in simParamDict.items()]
    tilings = GenTilingDefaultInputTup()

    return ops + sCounts + simParams + tilings

# functions for generating default input tuples
def GenInitialSpeciesCountDefaultInputTup(basin):
    '''
    generate some input tups that determine if the genetic toggle swtich system starts in basin A or basin B
    '''
    if basin.lower()=='a':
        iSCList = [4,16,1,0,0,0,0]
        iSCBList = [0,0,0,4,16,1,0]
    elif basin.lower()=='b':
        iSCList = [0,0,0,4,16,1,0]
        iSCBList = [4,16,1,0,0,0,0]
    else:
        raise ValueError('basin should be either a or b. basin: %s' % basin)

    iSCs = InitialSpeciesCounts(speciesCounts=iSCList)
    iSCBs = InitialSpeciesCountsBackward(speciesCounts=iSCBList)
    return [iSCs, iSCBs]

def GenFPTDefaultInputTup(speciesIDs):
    speciesTrackingList = ','.join([str(id) for id in speciesIDs])
    return [SimulationParameter(key='fptTrackingList',val=speciesTrackingList)]

def GenOParamDefaultInputTup():
    ops = [
        OrderParameter(type=0,
                       id=0,
                       speciesIDs=[0,1,2,3,4,5],
                       speciesCoefficients=[-1,-2,-2,1,2,2]),
        OrderParameter(type=0,
                       id=1,
                       speciesIDs=[0,1,2],
                       speciesCoefficients=[1,2,2]),
        OrderParameter(type=0,
                       id=2,
                       speciesIDs=[3,4,5],
                       speciesCoefficients=[1,2,2])]
    return ops

def GenOParamLimitDefaultInputTup(oparamID, kind, limit):
    if kind.lower()=='upper':
        key = 'orderParameterUpperLimitList'
    elif kind.lower()=='lower':
        key = 'orderParameterLowerLimitList'
    else:
        raise ValueError('kind should be either lower or upper. kind: %s' % kind)
    val = ':'.join([str(oparamID), str(limit)])
    return [SimulationParameter(key=key,val=val)]

def GenOParamLimitDefaultInputTupFromBasin(basin, oparamID=0, limit=27):
    if basin.lower()=='a':
        return GenOParamLimitDefaultInputTup(oparamID=oparamID, kind='upper', limit=limit)
    elif basin.lower()=='b':
        return GenOParamLimitDefaultInputTup(oparamID=oparamID, kind='lower', limit=-limit)
    else:
        raise ValueError('basin should be either a or b. basin: %s' % basin)

def GenTilingDefaultInputTup():
    tilings = [
        Tiling(id=0,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-27,27,13)),
        Tiling(id=19,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-25,25,13)),
        Tiling(id=1,
               orderParameterID=1,
               type=0,
               edges=np.arange(100)),
        Tiling(id=2,
               orderParameterID=2,
               type=0,
               edges=np.arange(100)),
        Tiling(id=3,
               orderParameterID=0,
               type=0,
               edges=np.arange(-100,100)),
        Tiling(id=7,
               orderParameterID=0,
               type=0,
               edges=np.linspace(-25,25,11))]

    tileCounts = [2] + list(range(4,21))[::4]
    for i in tileCounts:
        id = 100 + i
        numEdges = i+1
        tilings.append(Tiling(id=id,
                              orderParameterID=0,
                              type=0,
                              edges=np.linspace(-27, 27, numEdges)))
    return tilings

# functions for sweep input tuples
def GenSweepTup(ticks, label, paramFunc):
    inputTups = []
    for tick in ticks:
        inputTups.append(paramFunc(tick))
    return SweepTup(inputTupss=inputTups, label=label, labelVals=ticks)

def GenBarrierHeightInputTup(theta):
    '''
    generate a barrier height (theta) input tuple for a particular value of theta
    '''
    productionConstants = [ReactionRateConstant(reactionID=4, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=5, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=11, rateConstant=1.0*theta),
                           ReactionRateConstant(reactionID=12, rateConstant=1.0*theta)]
    degradationConstants = [ReactionRateConstant(reactionID=6, rateConstant=.25*theta),
                            ReactionRateConstant(reactionID=13, rateConstant=.25*theta)]
    return productionConstants + degradationConstants

def GenBarrierHeightSweepTup(ticks):
    '''
    generate a barrier height (theta) sweep tuple
    '''
    return GenSweepTup(ticks=ticks, label='theta_%.1e', paramFunc=GenBarrierHeightInputTup)