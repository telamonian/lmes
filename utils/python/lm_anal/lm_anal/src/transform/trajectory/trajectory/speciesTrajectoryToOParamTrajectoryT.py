from lm_anal.src.datum.trajectory.oparamTrajectory import OParamTrajectory
from lm_anal.src.datum.trajectory.speciesTrajectory import SpeciesTrajectory
from lm_anal.src.transform import BaseT
from lm_anal.src.spec import PropertyTransformSpec, PropertyTransformSpecs, TransformSpec, TransformSpecs

__all__ = ['SpeciesTrajectoryToOParamTrajectoryT']

class SpeciesTrajectoryToOParamTrajectoryT(BaseT):
    srcTypes = {SpeciesTrajectory}
    dstTypes = {OParamTrajectory}
    
    transformSpecs = TransformSpecs(TransformSpec(srcTypes=SpeciesTrajectory, dstTypes=OParamTrajectory, extraArgs='oparamIDs', requiredData='oparams',
                                                  propertyTransformSpecs=PropertyTransformSpecs(
                                                  PropertyTransformSpec(srcProps='species_count', dstProps='order_parameter_values', extraArgs='oparamIDs', type='special'),
                                                  PropertyTransformSpec(srcProps='', dstProps='number_order_parameters', extraArgs='oparamIDs', type='special'))))
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