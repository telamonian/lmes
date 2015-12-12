from lm_anal.src.datumABC.fflux import FFluxABC
from lm_anal.src.datumABC.hist import HistABC
from lm_anal.src.datumABC.pCloud import PCloudABC
from lm_anal.src.datumABC.trajectory import TrajectoryABC
from lm_anal.src.helper import Tupify

__all__ = ['datumABCDict', 'IsDatum', 'GetDatumABC', 'GetDatumABCSet', 'GetDatumPkgName',
                           'GetDatumStrABC', 'GetDatumStrABCSet',
                           'IsDatumType', 'GetDatumTypeABC', 'GetDatumTypeABCSet', 'GetDatumTypePkgName']

# TODO: organize all this crap into classes(?)

datumABCDict = {'fflux':FFluxABC, 'hist':HistABC, 'pCloud':PCloudABC, 'trajectory':TrajectoryABC}

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

def GetDatumABCSet(xs, frozen=False):
    '''
    return the (frozen)set of abstract base classes of a sequence of datum instances xs
    '''
    if frozen:
        return frozenset().union(map(GetDatumABC, Tupify(xs)))
    else:
        return set().union(map(GetDatumABC, Tupify(xs)))

def GetDatumPkgName(x):
    '''
    return the name of the package from whence datum instance x comes
    '''
    for pkgName,datumABC in datumABCDict.items():
        if isinstance(x, datumABC):
            return pkgName
    raise

# functions that take strings as arguments
def GetDatumStrABC(x):
    '''
    return the abstract base class of datum name string x
    '''
    return datumABCDict[x]

def GetDatumStrABCSet(xs, frozen=False):
    '''
    return the (frozen)set of abstract base classes of a sequence of datum name strings xs
    '''
    if frozen:
        return frozenset().union(map(GetDatumStrABC, Tupify(xs)))
    else:
        return set().union(map(GetDatumStrABC, Tupify(xs)))

# functions that take types (ie classes themselves) as arguments
def IsDatumType(tipe):
    '''
    test if x is an instance of one of the lm_anal datum types (that has a defined ABC)
    '''
    for datumABC in datumABCDict.values():
        if issubclass(tipe, datumABC):
            return True
    return False

def GetDatumTypeABC(tipe):
    '''
    return the abstract base class of datum type x
    '''
    for datumABC in datumABCDict.values():
        if issubclass(tipe, datumABC):
            return datumABC
    raise TypeError('type %s does not have a known abstract base class' % tipe)

def GetDatumTypeABCSet(tipes, frozen=False):
    '''
    return the (frozen)set of abstract base classes of a sequence of datum types xs
    '''
    if frozen:
        return frozenset().union(map(GetDatumTypeABC, Tupify(tipes)))
    else:
        return set().union(map(GetDatumTypeABC, Tupify(tipes)))

def GetDatumTypePkgName(tipe):
    '''
    return the name of the package from whence datum type x comes
    '''
    for pkgName,datumABC in datumABCDict.items():
        if issubclass(tipe, datumABC):
            return pkgName
    raise