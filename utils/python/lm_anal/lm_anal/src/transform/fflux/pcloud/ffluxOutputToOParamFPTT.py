from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.datum.fpt import OParamFPT
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.datumABC import datumABCDict, GetDatumTypeABC
from lm_anal.src.helper import Depth, Tupify
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['FFluxOutputToFFluxHistT']

class FFluxOutputToFFluxHistT(BaseT):
    srcTypes = frozenset({FFluxOutput}) 
    dstTypes = frozenset({OParamFPT})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes={FFluxOutput}, dstTypes={OParamFPT}, requiredArgs='tilingIDs', requiredData={'oparams', 'tilings'},
        propertyTransformSpecs=PropertyTransformSpecs(
            PropertyTransformSpec(dstProps='points', srcProps='basins', type='special'))))
    
    def genDatumKeyTuples(self, inputKey, **kwargs):
        '''
        (key, value) pair tuples version
        '''
        return (('InterfaceTilingID', inputKey), ('BinTilingIDs', Tupify(kwargs['tilingIDs'])))
    
    def genDatumKeyDelimited(self, inputKey, **kwargs):
        '''
        '_' and '_-_' delimited version
        '''
        return '_-_'.join([str(inputKey), 
                           '_'.join([str(tid) for tid in kwargs['tilingIDs']])])

    def tfd(self, srcs, dsts, keys=None, **kwargs):
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        for pT in self.propertyTransforms:
            if len(pT.srcTypes)==1:
                srcKeyData = self.findDataFromDatumInSet(srcs, next(iter(pT.srcTypes)))
            else:
                srcKeyData = self.findDataFromDatumInSet(srcs, pT.srcKeyType)
            
            if len(pT.dstTypes)==1:
                dstKeyData = self.findDataFromDatumInSet(dsts, next(iter(pT.dstTypes)))
            else:
                dstKeyData = self.findDataFromDatumInSet(dsts, pT.dstKeyType)
                    
            if keys==None:
                keys = srcKeyData.keys()
            
            # ughhh, this code
            if Depth(kwargs['tilingIDs'])!=2:
                # wrap shallow sets of tilingIDs in an extra container to ensure the following loop goes smoothly
                tilingIDss = (kwargs['tilingIDs'],)
            else:
                tilingIDss = kwargs['tilingIDs']
            
            for tilingIDs in tilingIDss:
                kwargs['tilingIDs'] = tilingIDs
                for key in keys:
                    srcDatum = srcKeyData[key]
                    srcsWithDatum = {srcDatum} | srcs
                    datumKey = self.genDatumKeyTuples(key, **kwargs)
                    # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
                    dstDatum = dstKeyData.initDatum(datumKey, full=srcDatum.full)
                    dstsWithDatum = {dstDatum} | dsts
                    
                    pT.transformProperties(srcs=srcsWithDatum, dsts=dstsWithDatum, **kwargs)