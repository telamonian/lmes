from lm_anal.src.spec import Spec

__all__ = ['DatumSpec']

class DatumSpec(Spec): #, metaclass=DatumPropertySpecMetaclass):
    keywords = {'dtype', 'name', 'paths', 'storageType', 'targetName', 'type'}
    
#     def __init__(self, name, **kwargs):
#         self.map = OrderedDict()
#         self.name = name
#         for key,val in kwargs.items():
#             self.map[key] = val
#         
#     def __delitem__(self, key):
#         del self.map[key]
#     
#     def __getitem__(self, key):
#         return self.map[key]
#     
#     def __setitem__(self, key, val):
#         if key in self.keywords:
#             self.map[key] = val
#         else:
#             raise AttributeError