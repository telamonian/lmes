from ast import literal_eval
import numpy as np

from lm_anal.src.helper.typeHelper import IsContainer

__all__ = ['ContainerEval', 'ContainerEvalReplaceNone', 'NumEval']

def ContainerEval(xs):
    if not IsContainer(xs):
        return NumEval(xs)
    evaledList = []
    for x in xs:
        evaledList.append(ContainerEval(x))
    return evaledList

def ContainerEvalReplaceNone(xs, replaceNone=float('-inf')):
    if not IsContainer(xs):
        evaledNum = NumEval(xs)
        return replaceNone if evaledNum is None else evaledNum
    evaledList = []
    for x in xs:
        evaledList.append(ContainerEvalReplaceNone(x))
    return evaledList

def NumEval(s):
    try:
        return literal_eval(s)
    except (SyntaxError, ValueError) as e:
        return s

# NumEvalVectorized = np.vectorize(NumEval, otypes=['O'])

if __name__=='__main__':
    testCon = [(('1.0e+00',), 3), (('1.0e+01',), 3), (('1.0e-01',), 3), (('1.6e+00',), 3), (('1.6e-01',), 3), (('2.5e+00',), 3), (('2.5e-01',), 3), (('4.0e+00',), 3), (('4.0e-01',), 3), (('6.3e+00',), 3), (('6.3e-01',), 3)]
    print(ContainerEval(testCon))