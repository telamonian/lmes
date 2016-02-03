import numpy as np

from lm_anal.src.propertyTransform.fflux.pcloud.ffluxPropToPCloudPropPT import FFluxPropToPCloudPropPT

__all__ = ['ProbabilityIToIPlusOneToTransitionProbabilityPT']

class ProbabilityIToIPlusOneToTransitionProbabilityPT(FFluxPropToPCloudPropPT):
    srcProps = frozenset({'probability_i_to_i_plus_one'})
    dstProps = frozenset({'transition_probability'})

    def getDstDatum(self, dataCaddy):
        return dataCaddy.dstDict['TransitionProbability']

    def getFieldDict(self, dataCaddy):
        return {'success': dataCaddy.edges[1:],
                'failure': dataCaddy.edges[0],
                'initial': dataCaddy.edges[:-1],
                'count': dataCaddy.ffluxBasinDatum.runs_per_phase[1:-1],
                'probability': dataCaddy.ffluxBasinDatum.probability_i_to_i_plus_one[1:-1],
                'cumulative_probability': np.cumprod(dataCaddy.ffluxBasinDatum.probability_i_to_i_plus_one[1:-1])}

    def getPointsShape(self, fieldDict):
        return len(fieldDict['probability'])

        # ffluxOutputDatum = srcDict['FFluxOutput']
        # ffluxBasinDatum = srcDict['FFluxBasin']
        # dstDatum = dstDict['TransitionProbability']
        #
        # direction = ffluxBasinDatum.direction
        # stride = 1 if direction==0 else -1
        # tiling = kwargs['tilings'][ffluxOutputDatum.tiling_id]
        # edges = np.asarray(tiling.getEdges()[0])[::stride]
        #
        # fieldValsDict = {'success': edges[1:],
        #                  'failure': edges[0],
        #                  'initial': edges[:-1],
        #                  'count': ffluxBasinDatum.run_per_phase[1:-1],
        #                  'probability': ffluxBasinDatum.probability_i_to_i_plus_one[1:-1],
        #                  'cumulative_probability': np.cumprod(ffluxBasinDatum.probability_i_to_i_plus_one[1:-1])}
        #
        # transition_probability = np.zeros(len(fieldValsDict['probability']), dtype=dstDatum.propertySpecs['points']['dtype'])
        # for field,vals in fieldValsDict.items():
        #     transition_probability[field][...] = vals
        #
        # dstDatum.setArray('transition_probability', transition_probability)