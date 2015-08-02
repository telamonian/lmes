import collections.abc
# from lm_anal.src.datum.fflux import FFluxBase
# import lm_anal.src.datum.fflux as fflux
# from lm_anal.src.datum.hist import HistBase
# from lm_anal.src.datum.trajectory import TrajectoryBase
# import lm_anal.src.datum.fflux as ffluxMod
# import lm_anal.src.datum.hist as histMod
# import lm_anal.src.datum.trajectory as trajectoryMod
from builtins import map

__all__ = ['IsContainer', 'Setify', 'Tupify']

def IsContainer(x):
    '''
    tests if x is an instance of one of the builtin container types
    '''
    return isinstance(x, (list,tuple,set,dict))#collections.abc.Container)

# def IsDatum(x):
#     '''
#     test if x is an instance of one of the lm_anal datum types (that has a defined ABC)
#     '''
#     for datumABC in datumABCs.values():
#         if isinstance(x, datumABC):
#             return True
#     return False
# 
# def GetDatumABC(x):
#     '''
#     return the abstract base class of datum instance x
#     '''
#     for datumABC in datumABCs.values():
#         if isinstance(x, datumABC):
#             return datumABC
#     raise
# 
# def GetDatumABCSet(xs):
#     '''
#     return the set of abstract base classes of a sequence of datum instances xs
#     '''
#     return set().union(*map(GetDatumABC, xs))
# 
# def GetDatumPkgName(x):
#     '''
#     return the name of the package from whence datum instance x comes
#     '''
#     for pkgName,datumABC in datumABCs.items():
#         if isinstance(x, datumABC):
#             return pkgName
#     raise

def Setify(x):
    '''
    if x is an instance of a builtin container, convert it to a set. Otherwise, place x into a set
    '''
    if IsContainer(x):
        return set(x)
    else:
        return {x}

def Tupify(x):
    '''
    if x is an instance of a builtin container, convert it to a tuple. Otherwise, place x into a tuple
    '''
    if IsContainer(x):
        return tuple(x)
    else:
        return (x,)