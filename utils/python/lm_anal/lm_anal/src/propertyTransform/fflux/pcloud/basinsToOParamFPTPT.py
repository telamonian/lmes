import numpy as np

from lm_anal.src.datumABC import datumABCDict, GetDatumStrABCSet
from lm_anal.src.propertyTransform.basePT import BasePT

__all__ = ['BasinsToOParamFPTPT']

class BasinsToOParamFPTPT(BasePT):
    srcABCs = GetDatumStrABCSet('fflux')
    dstABCs = GetDatumStrABCSet('pcloud')
    
    srcProps = frozenset({'basins'})
    dstProps = frozenset({'oparam_fpt'})
    
    def ptfd(self, srcDict, dstDict, **kwargs):
        ffluxDatum = srcDict['FFluxOutput']
        dstDatum = dstDict['OParamFPT']

        directionIDs = (0, 1)
        directions = ('FORWARD', 'BACKWARD')

        tiling = kwargs['tilings'][ffluxDatum.tiling_id]
        edges = np.asarray(tiling.getEdges()[0])
        oparam = kwargs['oparams'][tiling.order_parameter_id]
        reactionModel = kwargs['reactionModels']

        oparam_fpt = np.zeros(len(edges)*len(directionIDs), dtype=dstDatum.propertySpecs['points']['dtype'])
        for directionID,direction in zip(directionIDs, directions):
            initSpeciesCount = reactionModel.initial_species_counts if direction=='FORWARD' else reactionModel.initial_species_counts_backward
            initOParamCount = oparam.calc(np.atleast_2d(initSpeciesCount))

            basin = ffluxDatum.basins[direction]
            fluxOutOfTileZero = basin.flux_out_of_tile_zero

            stride = 1 if direction=='FORWARD' else -1
            for i,edge in enumerate(edges[::stride]):
                probabilityOneToIPlusOne = basin.probability_one_to_i_plus_one[i] if i>0 else 1

                time = 1.0/(fluxOutOfTileZero*probabilityOneToIPlusOne)
                # oparam_fpt fields: oparam_id, count, initial_count, time
                oparam_fpt[i + directionID*len(edges)] = (oparam.id,
                                                          edge,
                                                          initOParamCount,
                                                          time)

        dstDatum.setArray('oparam_fpt', oparam_fpt)