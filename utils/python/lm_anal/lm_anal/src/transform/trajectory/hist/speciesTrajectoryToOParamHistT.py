from lm_anal.src.datum.hist import OParamHist
from lm_anal.src.datum.trajectory import SpeciesTrajectory
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['SpeciesTrajectoryToOParamHistT']

class SpeciesTrajectoryToOParamHistT(BaseT):
    srcTypes = frozenset({SpeciesTrajectory})
    dstTypes = frozenset({OParamHist})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes=SpeciesTrajectory, dstTypes=OParamHist, requiredArgs='tilingIDs', requiredData={'oparams','tilings'},
                                        propertyTransformSpecs=PropertyTransformSpecs(
                                            PropertyTransformSpec(dstProps='order_parameter_values', srcProps='species_count', requiredArgs='tilingIDs', type='special'),
                                            PropertyTransformSpec(dstProps='tilings', srcProps='tilings', type='copy'))))
#     srcType = SpeciesTrajectory
#     dstType = OParamHist
#     
#     def __init__(self, src, dst, oparams, tilings, **kwargs):
#         '''
#         oparams: the complete oparams container
#         tilings: a list of all the tilings you want to use to define the bins of the resultant histogram (OParamHist)
#         '''
#         super().__init__(src, dst, oparams=oparams, tilings=tilings, **kwargs)
#         # combine all of the histograms we just created into a single 'sum' histogram
#         dst.getSum()