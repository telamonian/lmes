__all__ = ['IsContainer', 'Frozensetify', 'Setify', 'Tupify']

def IsContainer(x):
    '''
    tests if x is an instance of one of the builtin container types
    '''
    return isinstance(x, (dict, frozenset, list, set, tuple))#collections.abc.Container)

def Frozensetify(x):
    '''
    if x is an instance of a builtin container, convert it to a frozenset. Otherwise, place x into a frozenset
    '''
    if IsContainer(x):
        return frozenset(x)
    else:
        return frozenset({x})

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