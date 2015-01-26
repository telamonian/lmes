import numpy as np

def LogTicks(lowExp,highExp,base=10,resolution=5):
    '''
    returns log spaced ticks with minor tick resoltuion (in between the main ticks, which are spaced as base**lowExp - base**highExp)
    '''
    majorTickCount = (highExp - lowExp) + 1
    totalTickCount = (majorTickCount - 1)*resolution + majorTickCount
    return np.logspace(lowExp,highExp,base=base,num=totalTickCount)


# def OptionParserFactory(key, val=None, error=None):
#     def OptionParserClosure():
#         if val==
#     