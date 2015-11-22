ASCENDING = 0
DESCENDING = 1

from copy import deepcopy
import numpy as np

from lm_anal.src.datum.tiling.tiling import Tiling

class TilingLattice(Tiling):
    subtypeID = 0

    def combine(self, others):
        comboObj = deepcopy(self)
        objList = [comboObj] + others

        # check type
        for other in others:
            if other.type!=comboObj.type:
                raise Exception  # the type of all of the tilings you're trying to combine should be the same

        # set id and order_parameter_id
        ids = []
        orderParameterIDs = []
        for obj in objList:
            ids.append(obj.id)
            orderParameterIDs.append(obj.order_parameter_id)

        # -1 implies that ids should be checked instead of id, etc
        comboObj.id = -1
        comboObj.ids = np.array(ids)
        comboObj.order_parameter_id = -1
        comboObj.order_parameter_ids = np.array(orderParameterIDs)

        # combine all of the array (repeated) fields in the simplest possible way
        for attrName in [spec.name for spec in comboObj.propertySpecs.values() if spec['type']=='array']:
            attrVals = [obj.__getattribute__(attrName) for obj in objList]
            comboObj.__setattr__(attrName, np.concatenate(attrVals))

        # set rank
        comboObj.rank = len(comboObj.dims.shape)

        return comboObj

    def getEdgeIndices(self):
        return [np.arange(start,end) for start,end in self.getEdgeIndexStartEnds()]
    
    def getEdgeIndexStartEnds(self):
        '''
        based on what's in self.dims, generates a list of tuples of indices that can be used to transform the 1D array in which self.edges is stored into a list of lists, one list for every dim
        '''
        return [(int(np.sum(self.dims[:i])), int(np.sum(self.dims[:i + 1]))) for i in range(self.rank)]
    
    def getEdges(self):
        '''
        rolls the 1D self.edges array into an nD list-of-lists based on what's in self.dims
        '''
        return [self.edges[int(np.sum(self.dims[:i])):int(np.sum(self.dims[:i + 1]))] for i in range(self.rank)]
    
