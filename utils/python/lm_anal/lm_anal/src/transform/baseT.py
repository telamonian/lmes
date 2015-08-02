from lm_anal.src.datumABC import GetDatumTypeABCSet
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
            self.requiredKeywords = self.requiredKeywords | (transSpec['extraArgs'] | transSpec['requiredData'])
            self.propertyTransforms.append(PropertyTransforms(srcABCs=self.srcABCs, dstABCs=self.dstABCs, propertyTransformSpecs=transSpec['propertyTransformSpecs']))
    
    def tfd(self, src, dst, keys=None, **kwargs):
        '''
        generic tfd (transform from datum) method
        '''
        # check to make sure that we've got all of the data we need (in addition to src and dst)
        for keyword in self.requiredKeywords:
            if keyword not in kwargs:
                raise
        
        if keys==None:
            keys = src.keys()
            
        for key,srcDatum in zip(keys,src.valIter(keys)):
            # TODO: fix up 'full' keyword system. Here specifically, how should 'full' flag be set for Datum created from a Transform?
            dstDatum = dst.initDatum(key, full=srcDatum.full)
            for propertyTransform in self.propertyTransforms:
                propertyTransform.transformProperties(srcDatum, dstDatum, **kwargs)
        