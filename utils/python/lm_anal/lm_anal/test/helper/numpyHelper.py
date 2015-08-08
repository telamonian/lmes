import numpy as np
import scipy.special as spsp
import unittest

from lm_anal.src.helper import FastStack, FastHStack, FastVStack, timewith

class NumpyHelperTestCase(unittest.TestCase):
    def setUp(self):
        arrShape = (int(5e0), int(1e1))
        arrSize = int(np.product(arrShape))
        
        self.arrA = np.linspace(0, 10, num=arrSize).reshape(arrShape)
        self.arrB = np.linspace(12, 17, num=arrSize).reshape(arrShape)
        self.arrC = np.linspace(21, 23, num=arrSize).reshape(arrShape)
        
#         self.arrA = spsp.airy(np.linspace(0, 10, num=arrSize).reshape(arrShape))[0]
#         self.arrB = spsp.airye(np.linspace(0, 10, num=arrSize).reshape(arrShape))[0]
#         self.arrC = spsp.itairy(np.linspace(0, 10, num=arrSize).reshape(arrShape))[0]
        
    def test_FastHStack(self):
        '''
        test the FastHStack function from numpyHelper
        '''
        stackedArr = FastHStack(self.arrA, self.arrB, self.arrC)
        intendedStackedArr = np.hstack((self.arrA, self.arrB, self.arrC))
        try:
            testBool = np.allclose(stackedArr, intendedStackedArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool) #, msg='not allclose: %s\n%s' % (stackedArr.tolist(), intendedStackedArr.tolist()))
        
    def test_FastVStack(self):
        '''
        test the FastVStack function from numpyHelper
        '''
        stackedArr = FastVStack(self.arrA, self.arrB, self.arrC)
        intendedStackedArr = np.vstack((self.arrA, self.arrB, self.arrC))
        try:
            testBool = np.allclose(stackedArr, intendedStackedArr)
        except ValueError:
            testBool = False
        self.assertTrue(testBool) #, msg='not allclose: %s\n%s' % (stackedArr.tolist(), intendedStackedArr.tolist()))