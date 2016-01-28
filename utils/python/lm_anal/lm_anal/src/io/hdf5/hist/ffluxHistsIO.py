from lm_anal.src.io.hdf5 import HDF5IO
from lm_anal.src.spec.io.hdf5 import HDF5IOSpec, HDF5IOSpecs
from lm_anal.src.io.hdf5.hist.oparamHistsIO import OParamHistsIO

class FFluxHistsIO(OParamHistsIO):
    hdf5RootPath = 'FFluxHists'
    hdf5Specs = HDF5IOSpecs(HDF5IOSpec(name='basin_n_order_parameter_values', fullOnly=False, IOType=OParamHistsIO, subKey='BasinHists', type='subData'),
                            HDF5IOSpec(name='basin_weights', fullOnly=False, subKey='BasinWeights', type='dataset'),
                            HDF5IOSpec(name='h', fullOnly=False, subKey='H', type='histogram'),
                            HDF5IOSpec(name='phase_weights', fullOnly=False, subKey='PhaseWeights', type='dataset'),
                            HDF5IOSpec(name='phase_n_order_parameter_values', fullOnly=False, IOType=OParamHistsIO, subKey='PhaseNHists', type='subData'),
                            HDF5IOSpec(name='trajectory_phase_map', fullOnly=True, subKey='TrajectoryPhaseMap', type='dataset'))