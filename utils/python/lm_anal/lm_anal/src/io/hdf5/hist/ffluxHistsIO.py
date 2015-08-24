from lm_anal.src.io.hdf5 import HDF5IO, HDF5Spec, HDF5Specs
from lm_anal.src.io.hdf5.hist.oparamHistsIO import OParamHistsIO

class FFluxHistsIO(OParamHistsIO):
    hdf5RootPath = 'FFluxHists'
    hdf5Specs = HDF5Specs(HDF5Spec(name='basin_weights', fullOnly=False, subKey='BasinWeights', type='dataset'),
                          HDF5Spec(name='h', fullOnly=False, subKey='H', type='histogram'),
                          HDF5Spec(name='phase_weights', fullOnly=False, subKey='PhaseWeights', type='dataset'),
                          HDF5Spec(name='phase_zero_order_parameter_values', fullOnly=False, IOType=OParamHistsIO, subKey='PhaseZeroHists', type='subData'),
                          HDF5Spec(name='phase_n_order_parameter_values', fullOnly=False, IOType=OParamHistsIO, subKey='PhaseNHists', type='subData'),
                          HDF5Spec(name='trajectory_phase_map', fullOnly=True, subKey='TrajectoryPhaseMap', type='dataset'))