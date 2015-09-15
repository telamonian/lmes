from collections import OrderedDict
import numpy as np

from lm_anal.src.helper import Frozensetify

__all__ = ['MagicGrid']

class MagicGrid(object):
    pass
#     def __init__(self, *args, **kwargs): 
#         OrderedDict.__init__(self, *args, **kwargs)
#         self.elemDict = OrderedDict()
#         self.setElems()
#         self.sortElems()
#     
#     def __setitem__(self, key, val):
#         key = Frozensetify(key)
#         self.addElemsFromKey(key)
#         OrderedDict.__setitem__(self, key, val)
#     
#     def addElem(self, elem):
#         elemValDict = self.elemDict.get(elem[0], OrderedDict())
#         elemValDict[elem[1:]] = elemValDict.get(elem[1:], 0) + 1
#         self.elemDict[elem[0]] = elemValDict
#     
#     def addElemsFromKey(self, key):
#         for elem in key:
#             self.addElem(elem)
#     
#     def clearElems(self):
#         del self.elemDict
#         self.elemDict = OrderedDict()
#     
#     def getElemVals(self, elemKey):
#         '''
#         assuming that all of the keys in your magicDict are in the form frozenset(('elemKey1', 'elemVal1'), ('elemKey2', 'elemVal2'), ...), this returns all of the elemVals corresponding to a particular elemKey
#         '''
#         elemVals = []
#         for keySet in self.keys():
#             for elem in (elem for elem in keySet if len(elem) > 1 and elem[0]==elemKey):
#                 elemVals.append(elem[1:])
#     
#     def getGrid(self):
#         gridElems, singletonElems = self.getGridElemsWithSingletons()
#         gridElemKeys = [gridElem[0] for gridElem in gridElems]
#         gridElemVals = [gridElem[1] for gridElem in gridElems]
#         grid = np.zeros([len(elemVals) for elemVals in gridElemVals], dtype=object)
# #         elemValMeshgrid = np.meshgrid(*gridElemVals)
#         it = np.nditer(grid, flags=['multi_index', 'refs_ok'], op_flags=[['writeonly']])
#         for gridSpot in it:
#             elemVals = []
#             for i,elemValList in enumerate(gridElemVals):
#                 elemVals.append(elemValList[it.multi_index[i]])
#             gridKey = []
#             for elemKey,elemVal in zip(gridElemKeys, elemVals):
#                 gridKey.append((elemKey,) + elemVal)
#             gridKey+=singletonElems
#             try:
# #                 print(gridKey)
#                 gridVal = self[gridKey]
#             except KeyError:
#                 gridVal = None
#             if isinstance(gridVal, MagicDict):
#                 raise
#             grid[it.multi_index] = gridVal
#         return grid, gridElemKeys, gridElemVals
#     
#     def getGridElems(self, excludeSingletons=True):
#         self.sortElems()
#         gridElems = []
#         for elemKey, elemValDict in self.elemDict.items():
#             if excludeSingletons and len(elemValDict)<=1:
#                 continue
#             gridElems.append((elemKey, list(elemValDict.keys())))
#         return gridElems
#     
#     def getGridElemsWithSingletons(self):
#         self.sortElems()
#         gridElems = []
#         singletonElems = []
#         for elemKey, elemValDict in self.elemDict.items():
#             if len(elemValDict)<=1:
#                 singletonElems.append((elemKey,) + next(elemValDict.keys().__iter__()))
#             else:
#                 gridElems.append((elemKey, list(elemValDict.keys())))
#         return gridElems, singletonElems