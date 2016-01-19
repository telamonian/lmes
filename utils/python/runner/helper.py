import numpy as np
import os

def LogTicks(lowExp,highExp,base=10,resolution=5):
    '''
    returns log spaced ticks with minor tick resoltuion (in between the main ticks, which are spaced as base**lowExp - base**highExp)
    '''
    majorTickCount = (highExp - lowExp) + 1
    totalTickCount = (majorTickCount - 1)*resolution + majorTickCount
    return np.logspace(lowExp,highExp,base=base,num=totalTickCount)

def PathJoin(path, *paths):
    '''
    exactly like os.path.join, except that it *will* join absolute paths without throwing out prior path elements
    '''
    return os.path.join(path, *[path.lstrip(os.sep) for path in paths])

def InList(s, l):
    '''
    s: a string
    l: a list of strings
    returns true if s is `in` any of the strings in l
    otherwise, returns false
    '''
    for lString in l:
        if s in lString:
            return True
    return False
    
# def OptionParserFactory(key, val=None, error=None):
#     def OptionParserClosure():
#         if val==
#     