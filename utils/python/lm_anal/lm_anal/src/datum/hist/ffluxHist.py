import numpy as np

# from lm_anal.src.datum.hist.ffluxHists import FFluxHists
from lm_anal.src.datum.hist.oparamHist import OParamHist
from lm_anal.src.spec import DatumSpec, DatumSpecs

__all__ = ['FFluxHist']
    
class FFluxHist(OParamHist):
    propertySpecs = DatumSpecs(DatumSpec(name='basin_order_parameter_values', paths=('basin_order_parameter_values',), type='subData'),
                               DatumSpec(name='bin_tiling_id', dtype='int', paths=('bin_tiling_id',), storageType='numpy', type='array'),
                               DatumSpec(name='interface_tiling_id', dtype='int', paths=('interface_tiling_id',), storageType='numpy', type='array'),
                               DatumSpec(name='phase_weights', columnLabels=('weight', 'basin', 'phase'), dtype=('float','int','int'), paths=('tiling_ids',), storageType='numpy', type='array'),
                               DatumSpec(name='phase_zero_order_parameter_values', paths=('phase_zero_order_parameter_values',), type='subData'),
                               DatumSpec(name='time_step', dtype='float', paths=('time_step',), storageType='default', type='scalar'),
                               DatumSpec(name='trajectory_phase_map', columnLabels=('trajectory_id', 'basin', 'phase'), dtype=('int','int','int'), paths=('trajectory_phase_map',), storageType='numpy', type='array'))
    
    # add some alias specs
    propertySpecs.addAlias(name='phase_0_order_parameter_values', targetName='phase_zero_order_parameter_values')
    
    def __init__(self, full=False):
        super().__init__(full=full)
        
        from lm_anal.src.datum.hist.ffluxHists import FFluxHists
        self.basin_order_parameter_values = FFluxHists()
        self.phase_zero_order_parameter_values = FFluxHists()