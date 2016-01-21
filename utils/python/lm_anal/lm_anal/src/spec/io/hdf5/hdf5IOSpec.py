from lm_anal.src.spec import Spec

__all__ = ['HDF5IOSpec']

class HDF5IOSpec(Spec):
    keywords = {'DataType', 'IOType', 'fullOnly', 'name', 'o', 'subKey', 'type'}
    
    defaultKeyValDict = {'fullOnly': False,
                         'o': None}