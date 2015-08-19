from lm_anal.src.datum.fflux import FFluxOutput
from lm_anal.src.datum.hist import FFluxHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.datumABC import datumABCDict, GetDatumTypeABC
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['FFluxOutputToFFluxHistT']

class FFluxOutputToFFluxHistT(BaseT):
    srcTypes = frozenset({FFluxOutput}) 
    dstTypes = frozenset({FFluxHist})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes={FFluxOutput}, dstTypes={FFluxHist}, requiredArgs='tilingIDs', requiredData={'oparams', 'tilings'},
                                                  propertyTransformSpecs=PropertyTransformSpecs(
                                                  PropertyTransformSpec(dstProps='basin_weights', srcProps='basins', type='special'),
                                                  PropertyTransformSpec(dstProps='phase_weights', srcProps='basins', type='special'),
                                                  PropertyTransformSpec(dstProps='order_parameter_values', srcProps='trajectories', requiredArgs='tilingIDs', type='special'))))