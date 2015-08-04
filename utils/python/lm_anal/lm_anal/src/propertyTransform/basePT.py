class BasePT(object):
    srcABCs = frozenset()
    dstABCs = frozenset()
    srcTypes = frozenset()
    dstTypes = frozenset()
    srcProps = frozenset()
    dstProps = frozenset()
    
    def __init__(self, **kwargs):
        pass
    
    def checkSrcProps(self, srcProps):
        return srcProps==self.srcProps
    
    def checkDstProps(self, dstProps):
        return dstProps==self.dstProps
    
    def checkProps(self, srcProps, dstProps):
        return srcProps==self.srcProps and dstProps==self.dstProps
    
    def ptfd(self, srcDatum, dstDatum, **kwargs):
        '''
        generic ptfd (property transform from datum) method
        '''
        pass