from lm_anal.src.datumABC import GetDatumTypeABCSet
from lm_anal.src.helper import FindInstanceInSet
from lm_anal.src.propertyTransform import PropertyTransforms

class BaseT(object):
    srcTypes = None
    dstTypes = None
    
    transformSpecs = None
    
    @property
    def srcABCs(self):
        return GetDatumTypeABCSet(self.srcTypes)
    
    @property
    def dstABCs(self):
        return GetDatumTypeABCSet(self.dstTypes)

    @classmethod
    def checkSrcABCs(cls, srcABCs):
        return srcABCs==cls.srcABCs
    
    @classmethod
    def checkDstABCs(cls, dstABCs):
        return dstABCs==cls.dstABCs
    
    @classmethod
    def checkABCs(cls, srcABCs, dstABCs):
        return srcABCs==cls.srcABCs and dstABCs==cls.dstABCs
    
    @classmethod
    def checkSrcTypes(cls, srcTypes):
        return srcTypes==cls.srcTypes
    
    @classmethod
    def checkDstTypes(cls, dstTypes):
        return dstTypes==cls.dstTypes
    
    @classmethod
    def checkTypes(cls, srcTypes, dstTypes):
        return srcTypes==cls.srcTypes and dstTypes==cls.dstTypes
    
    def __init__(self):     #, src, dst, keys=None, **kwargs):
#         pkg = import_module('.'.join(self.__class__.__module__.split('.')[:-1]))
#         self.propertyTransformPkgDict = pkg.propertyTransformPkgDict
#         for propertyTransformPkg in self.propertyTransformPkgDict.values():
#             if self.checkDstABCs(propertyTransformPkg.dstABCs):
#                 self.propertyTransformPkg = propertyTransformPkg
#                 break
#             raise
        
        self.propertyTransforms = []
        self.requiredKeywords = set()
        for transSpec in self.transformSpecs.values():
            self.requiredKeywords = self.requiredKeywords | (transSpec['requiredArgs'] | transSpec['requiredData'])
            self.propertyTransforms.append(PropertyTransforms(**transSpec))
#                                                               srcTypes=transSpec['srcTypes'], dstTypes=transSpec['dstTypes'],
#                                                               srcDataTypes=transSpec['srcDataTypes'], dstTypes=transSpec['dstTypes'],
#                                                               srcTypes=transSpec['srcTypes'], dstTypes=transSpec['dstTypes'], 
#                                                               propertyTransformSpecs=transSpec['propertyTransformSpecs']))
    
    @staticmethod
    def findDataFromDatumInSet(datumSet, Tipe):
        for obj in datumSet:
            try:
                if issubclass(obj.datumType, Tipe):
                    return obj
            except AttributeError:
                pass
        # we got here because no appropriate data instance was found
        raise
    
    def genDatumKey(self, inputKey, **kwargs):
        '''
        overridable method that allows for the keys of transform-produced datums to be customized at the class level
        '''
        return inputKey
        
    def tfd(self, srcs, dsts, keys=None, **kwargs):
        '''
        generic tfd (transform from datum) method
        '''
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        for pT in self.propertyTransforms:
            if len(pT.srcTypes)==1:
                srcKeyData = self.findDataFromDatumInSet(srcs, next(iter(pT.srcTypes)))
            else:
                srcKeyData = self.findDataFromDatumInSet(srcs, pT.srcKeyType)
            
            if len(pT.dstTypes)==1:
                dstKeyData = self.findDataFromDatumInSet(dsts, next(iter(pT.dstTypes)))
            else:
                dstKeyData = self.findDataFromDatumInSet(dsts, pT.dstKeyType)
                    
            if keys==None:
                keys = srcKeyData.keys()
                
            for key in keys:
                srcDatum = srcKeyData[key]
                srcsWithDatum = {srcDatum} | srcs
                datumKey = self.genDatumKey(key, **kwargs)
                # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
                dstDatum = dstKeyData.initDatum(datumKey, full=srcDatum.full)
                dstsWithDatum = {dstDatum} | dsts
                
                pT.transformProperties(srcs=srcsWithDatum, dsts=dstsWithDatum, **kwargs)
            