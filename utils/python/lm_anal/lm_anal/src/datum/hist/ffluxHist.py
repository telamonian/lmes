import numpy as np

# from lm_anal.src.datum.hist.ffluxHists import FFluxHists
from lm_anal.src.datum.hist.oparamHist import OParamHist
from lm_anal.src.helper import LazyClass
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['FFluxHist']

lzFFluxHists = LazyClass(modName='lm_anal.src.datum.hist.ffluxHists', clsName='FFluxHists')

class FFluxHist(OParamHist):
    propertySpecs = DatumSpecs(DatumSpec(name='basin_weights', columnLabels=('weight', 'basin_id'), dtype=('float','int'), paths=('basin_weights',), storageType='numpy', type='array'),
                               DatumSpec(name='bin_tiling_id', dtype='int', paths=('bin_tiling_id',), storageType='numpy', type='array'),
                               DatumSpec(name='interface_tiling_id', dtype='int', paths=('interface_tiling_id',), storageType='numpy', type='array'),
                               DatumSpec(name='phase_weights', columnLabels=('weight', 'basin_id', 'phase_id'), dtype=('float','int','int'), paths=('basin_weights',), storageType='numpy', type='array'),
                               DatumSpec(name='phase_n_order_parameter_values', paths=('phase_n_order_parameter_values',), subDataType=lzFFluxHists, type='subData'),
                               DatumSpec(name='phase_zero_order_parameter_values', paths=('phase_zero_order_parameter_values',), subDataType=lzFFluxHists, type='subData'),
                               DatumSpec(name='time_step', dtype='float', paths=('time_step',), storageType='default', type='scalar'),
                               DatumSpec(name='trajectory_phase_map', columnLabels=('trajectory_id', 'basin_id', 'phase_id'), dtype=('int','int','int'), paths=('trajectory_phase_map',), storageType='numpy', type='array'))
    
    # add some alias specs
    propertySpecs.addAlias(name='phase_0_order_parameter_values', targetName='phase_zero_order_parameter_values')
    
    def __init__(self, full=False):
        super().__init__(full=full)
        
        self.initSubData()