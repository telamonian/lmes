from collections import OrderedDict
import numpy as np

from lm_anal.src.helper import ContainerEval, Frozensetify

__all__ = ['MagicDict']

class MagicDict(OrderedDict):
    gridDType = [('obj', 'O'), ('label', 'O')]

    @staticmethod
    def getElemFromKey(elemKey, key):
        for elem in key:
            if elemKey==elem[0]:
                return elem[1]
        # we got here cuz we didn't find anything
        raise KeyError('could not find elemKey: %s in key: %s' % (elemKey, key))

    def __init__(self, *args, **kwargs): 
        OrderedDict.__init__(self, *args, **kwargs)
        self.elemDict = OrderedDict()
        self.setElems()
        self.sortElems()
    
    def __delitem__(self, key):
        key = Frozensetify(key)
        self.popElemsFromKey(key)
        OrderedDict.__delitem__(self, key)
    
    def __getitem__(self, keyTups):
        # ignore the order of the (elemKey, elemVal) tuples
        keySet = Frozensetify(keyTups)
        try:
            return self._get(keySet=keySet)
        except KeyError:
            try:
                # _get may have failed because the user entered a tup instead of a tup-of-tups for the key, so try to fix that
                keySet = Frozensetify((keyTups,))
                return self._get(keySet=keySet)
            except TypeError:
                # maybe make this a less soft fail
                return None

    def _get(self, keySet):
        try:
            # if you specified an **exact** key, return the matching value
            return self._getExact(keySet=keySet)
        except KeyError:
            # otherwise, if your key partially matches any keys in the magicDict, return a new magicDict with only the matching items
            return self._query(keySet=keySet)

    def _getExact(self, keySet):
        return OrderedDict.__getitem__(self, keySet)

    def _query(self, keySet):
        newDict = MagicDict()
        anyFound = False
        for oldKeySet,oldVal in self.items():
            if keySet<=oldKeySet:
                newDict[oldKeySet] = oldVal
                anyFound = True
        if anyFound:
            newDict.sortElems()
            return newDict
        else:
            raise KeyError("keySet: %s did not partially match any keys in this %s" % (keySet, self.__class__.__name__))

    def __setitem__(self, key, val):
        key = Frozensetify(key)
        self.addElemsFromKey(key)
        OrderedDict.__setitem__(self, key, val)
    
    def addElem(self, elem):
        elemValDict = self.elemDict.get(elem[0], OrderedDict())
        elemValDict[elem[1:]] = elemValDict.get(elem[1:], 0) + 1
        self.elemDict[elem[0]] = elemValDict
    
    def addElemsFromKey(self, key):
        for elem in key:
            self.addElem(elem)
    
    def clearElems(self):
        del self.elemDict
        self.elemDict = OrderedDict()
    
    def getElemVals(self, elemKey):
        '''
        assuming that all of the keys in your magicDict are in the form frozenset(('elemKey1', 'elemVal1'), ('elemKey2', 'elemVal2'), ...), this returns all of the elemVals corresponding to a particular elemKey
        '''
        elemVals = []
        for keySet in self.keys():
            for elem in (elem for elem in keySet if len(elem) > 1 and elem[0]==elemKey):
                elemVals.append(elem[1:])
    
    def getGrid(self):
        gridElems, singletonElems = self.getGridElemsWithSingletons()
        gridElemKeys = [gridElem[0] for gridElem in gridElems]
        gridElemVals = [gridElem[1] for gridElem in gridElems]

        gridShape = [len(elemVals) for elemVals in gridElemVals]
        # a lot of later code expects grid to be 2D, so coerce 1D grids into (1,X) grids
        if len(gridShape)==0:
            gridShape = (1, 1)
        elif len(gridShape)==1:
            gridShape.append(1)
        grid = np.zeros(gridShape, dtype=self.gridDType)
#         elemValMeshgrid = np.meshgrid(*gridElemVals)
        it = np.nditer(grid, flags=['multi_index', 'refs_ok'])
        for gridSpot in it:
            elemVals = []
            for i,elemValList in enumerate(gridElemVals):
                elemVals.append(elemValList[it.multi_index[i]])
            gridKey = []
            for elemKey,elemVal in zip(gridElemKeys, elemVals):
                gridKey.append((elemKey,) + elemVal)
            grid[it.multi_index]['label'] = tuple(gridKey)
            gridKey+=singletonElems
            try:
                gridVal = self[gridKey]
            except KeyError:
                gridVal = None
            if isinstance(gridVal, MagicDict):
                raise
            grid[it.multi_index]['obj'] = gridVal
        # self.sortGrid(grid)
        return grid, singletonElems
    
    def getGridElems(self, excludeSingletons=True):
        self.sortElems()
        gridElems = []
        for elemKey, elemValDict in self.elemDict.items():
            if excludeSingletons and len(elemValDict)<=1:
                continue
            gridElems.append((elemKey, list(elemValDict.keys())))
        return gridElems
    
    def getGridElemsWithSingletons(self):
        self.sortElems()
        gridElems = []
        singletonElems = []
        for elemKey, elemValDict in self.elemDict.items():
            if len(elemValDict)<=1:
                singletonElems.append((elemKey,) + next(elemValDict.keys().__iter__()))
            else:
                gridElems.append((elemKey, list(elemValDict.keys())))
        return gridElems, singletonElems

    def peek(self):
        '''
        return the "first" entry from self
        '''
        return next(self.values().__iter__())

    def popElem(self, elem):
        elemValDict = self.elemDict[elem[0]]
        
        elemValDict[elem[1:]]-=1
        
        if elemValDict[elem[1:]] < 0:
            raise
        elif elemValDict[elem[1:]]==0:
            retVal = elemValDict.pop(elem[1:])
            if len(elemValDict)==0:
                retVal = self.elemDict.pop(elem[0])
        else:
            retVal = elemValDict[elem[1:]]
        
        return retVal
    
    def popElemsFromKey(self, key):
        for elem in key:
            self.popElem(elem)
    
    def setElems(self):
        self.clearElems()
        for key in self.keys():
            self.addElemsFromKey(key)
    
    def sortElems(self):
        for key, elemValDict in self.elemDict.items():
            self.elemDict[key] = OrderedDict(sorted(elemValDict.items(), key=lambda item: ContainerEval(item[0])))
        self.elemDict = OrderedDict(sorted(self.elemDict.items(), key=lambda item: ContainerEval(item[0])))

    # @staticmethod
    # def evalElems(elems):
    #     return [(elem[0], NumEval(elem[1])) for elem in elems]

    # @staticmethod
    # def sortGrid(grid):
    #     for i in np.arange(len(grid.shape)):
    #         grid = grid[np.lexsort(NumEval(grid['label']), axis=i)]