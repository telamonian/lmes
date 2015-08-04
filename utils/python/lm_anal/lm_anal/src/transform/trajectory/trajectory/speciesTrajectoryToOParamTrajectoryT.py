from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs
from lm_anal.src.transform import BaseT

__all__ = ['SpeciesTrajectoryToOParamTrajectoryT']

class SpeciesTrajectoryToOParamTrajectoryT(BaseT):
    srcTypes = frozenset({SpeciesTrajectory})
    dstTypes = frozenset({OParamTrajectory})
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes=SpeciesTrajectory, dstTypes=OParamTrajectory, requiredArgs='oparamIDs', requiredData='oparams',
                                                  propertyTransformSpecs=PropertyTransformSpecs(
                                                  PropertyTransformSpec(dstProps='number_entries', srcProps='number_entries', type='copy'),
                                                  PropertyTransformSpec(dstProps='number_order_parameters', srcProps='', requiredArgs='oparamIDs', type='special'),                                              
                                                  PropertyTransformSpec(dstProps='order_parameter_values', srcProps='species_count', requiredArgs='oparamIDs', type='special'),
                                                  PropertyTransformSpec(dstProps='time', srcProps='time', type='copy'),
                                                  PropertyTransformSpec(dstProps='trajectory_id', srcProps='trajectory_id', type='copy'))))
#     srcType = SpeciesTrajectory
#     dstType = OParamTrajectory
#     
#     def __init__(self, src, dst, oparam, **kwargs):
#         self.oparam = oparam
#         super().__init__(src, dst, oparam=oparam, **kwargs)
# 
#     def execTransform(self, src, dst):
#         for key,datum in dst:
#             self.propTrans(srcDatum=src[key], dstDatum=datum)
#             datum.setScalar('number_order_parameters', self.oparam.numberOrderParameters)