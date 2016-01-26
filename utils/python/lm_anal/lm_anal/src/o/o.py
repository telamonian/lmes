from enum import IntEnum

__all__ = ['O']

complexityCategories = ['constant',
                        'log',
                        'linear',
                        'logLinear',
                        'polynomial',
                        'logPolynomial',
                        'exponential']

O = IntEnum('O', ''.join(complexityCategories), module=__name__, qualname=__name__+'.O')

# class O(IntEnum):
#     constant = 0
#     linear = 1
#     polynomial = 2
#     exponential = 3