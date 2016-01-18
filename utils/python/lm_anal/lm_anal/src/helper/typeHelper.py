import ast

__all__ = ['Depth', 'FindInstanceInSet','IsContainer', 'Frozensetify', 'NumifyString', 'Setify', 'Tupify']

def Depth(x):
    '''
    a simple, non-robust function for determining how many levels a homogenous list-of-lists-of-lists-of... has
    '''
    depth = 0
    while True:
        if IsContainer(x):
            depth+=1
            x = x[0]
        else:
            break
    return depth

def FindInstanceInSet(sett, Tipe, raiseNotFound=False):
    '''
    return the "first" instance of Type Tipe in set sett
    '''
    for obj in sett:
        if isinstance(obj, Tipe):
            return obj
    if raiseNotFound:
        raise
    # return None

def IsContainer(x):
    '''
    tests if x is an instance of one of the builtin container types
    '''
    return isinstance(x, (dict, frozenset, list, set, tuple))#collections.abc.Container)

# functions to compel generic data into a particular sequence Type
def Frozensetify(x):
    '''
    if x is an instance of a builtin container, convert it to a frozenset. Otherwise, place x into a frozenset
    '''
    if IsContainer(x):
        return frozenset(x)
    else:
        return frozenset({x})

def NumifyString(x):
    try:
        # first try a simple int() conversion...
        return int(x)
    except ValueError:
        try:
            # ...then see if it can eval to any numeric type...
            return ast.literal_eval(x)
        except ValueError:
            # ... and if everything fails just return the original string
            return x

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