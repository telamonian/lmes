class Transform(object):
    dataAttr = None
#     simData = None
    
#     def __init__(self, sim, **kwargs):
#         self.sim = sim
#         for key,val in self.sim.dataDict.items():
#             self.__setattr__(key, val)
    
    def _transformDatum(self, datum):
        pass
            
    def transformDatum(self):
        for datum in self.sim.__getattribute__(self.__class__.dataAttr).sffHDF5():
            self._transformDatum(datum)