from lm_anal.src.transform.transform import Transform

class SpeciesToOParamT(Transform):
    def __init__(self, src, dst, **kwargs):
        self.tfd(src, dst, **kwargs)
        
    def tfd(self, src, dst, **kwargs):
        for key,datum in src:
            dst.initDatum(key, )
    
    def _tfd(self, src, dst, **kwargs):
        '''
        tfd: transform from data
        '''
        pass