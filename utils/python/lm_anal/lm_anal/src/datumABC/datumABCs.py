from lm_anal.src.datumABC.fflux import FFluxABC
from lm_anal.src.datumABC.hist import HistABC
from lm_anal.src.datumABC.trajectory import TrajectoryABC

__all__ = ['datumABCDict', 'IsDatum', 'GetDatumABC', 'GetDatumABCSet', 'GetDatumPkgName',
                           'IsDatumType', 'GetDatumTypeABC', 'GetDatumTypeABCSet', 'GetDatumTypePkgName']

datumABCDict = {'fflux':FFluxABC, 'hist':HistABC, 'trajectory':TrajectoryABC}

# functions that take instances as arguments
def IsDatum(x):
    '''
    test if x is an instance of one of the lm_anal datum types (that has a defined ABC)
    '''
    for datumABC in datumABCDict.values():
        if isinstance(x, datumABC):
            return True
    return False

def GetDatumABC(x):
    '''
    return the abstract base class of datum instance x
    '''
    for datumABC in datumABCDict.values():
        if isinstance(x, datumABC):
            return datumABC
    raise

def GetDatumABCSet(xs):
    '''
    return the set of abstract base classes of a sequence of datum instances xs
    '''
    return set().union(map(GetDatumABC, xs))

def GetDatumPkgName(x):
    '''
    return the name of the package from whence datum instance x comes
    '''
    for pkgName,datumABC in datumABCDict.items():
        if isinstance(x, datumABC):
            return pkgName
    raise

# functions that take types (ie classes themselves) as arguments
def IsDatumType(x):
    '''
    test if x is an instance of one of the lm_anal datum types (that has a defined ABC)
    '''
    for datumABC in datumABCDict.values():
        if issubclass(x, datumABC):
            return True
    return False

def GetDatumTypeABC(x):
    '''
    return the abstract base class of datum type x
    '''
    for datumABC in datumABCDict.values():
        if issubclass(x, datumABC):
            return datumABC
    raise

def GetDatumTypeABCSet(xs):
    '''
    return the set of abstract base classes of a sequence of datum types xs
    '''
    return set().union(map(GetDatumTypeABC, xs))

def GetDatumTypePkgName(x):
    '''
    return the name of the package from whence datum type x comes
    '''
    for pkgName,datumABC in datumABCDict.items():
        if issubclass(x, datumABC):
            return pkgName
    raise