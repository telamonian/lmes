import numpy as np

from lm_anal.src.datumABC import GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['ProbabilityIToIPlusOneToTransitionProbabilityPT']

class ProbabilityIToIPlusOneToTransitionProbabilityPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('pcloud')
    
    srcProps = frozenset({'probability_i_to_i_plus_one'})
    dstProps = frozenset({'transition_probability'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        ffluxOutputDatum = srcDict['FFluxOutput']
        ffluxBasinDatum = srcDict['FFluxBasin']
        dstDatum = dstDict['TransitionProbability']

        direction = ffluxBasinDatum.direction
        stride = 1 if direction==0 else -1
        tiling = kwargs['tilings'][ffluxOutputDatum.tiling_id]
        edges = np.asarray(tiling.getEdges()[0])[::stride]

        fieldValsDict = {'failure': edges[0],
                        'initial': edges[:-1],
                        'success': edges[1:],
                        'probability': ffluxBasinDatum.probability_i_to_i_plus_one[1:-1]}

        transition_probability = np.zeros(len(fieldValsDict['probability']), dtype=dstDatum.propertySpecs['points']['dtype'])
        for field,vals in fieldValsDict.items():
            transition_probability[field][...] = vals

        dstDatum.setArray('transition_probability', transition_probability)