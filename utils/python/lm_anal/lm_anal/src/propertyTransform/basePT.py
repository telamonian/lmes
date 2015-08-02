class BasePT(object):
    srcProps = None
    dstProps = None
    
    @classmethod
    def checkSrcProps(cls, srcProps):
        return srcProps==cls.srcProps
    
    @classmethod
    def checkDstProps(cls, dstProps):
        return dstProps==cls.dstProps
    
    @classmethod
    def checkProps(cls, srcProps, dstProps):
        return srcProps==cls.srcProps and dstProps==cls.dstProps
    
    def __init__(self, **kwargs):
        pass
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        '''
        generic ptfd (property transform from datum) method
        '''
        pass