from lm_anal.src.transform import transformDict

class Transforms(object):
    def __init__(self, src, dst, **kwargs):
        self.transform = transformDict[(src.datumType, dst.datumType)]
        self.execTransform(src, dst, **kwargs)
        
    def execTransform(self, src, dst, **kwargs):
        self.transform(src, dst, **kwargs)