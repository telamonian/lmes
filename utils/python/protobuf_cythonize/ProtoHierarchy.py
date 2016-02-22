import os
from ProtoHdr import ProtoHdr

class ProtoHierarchy(object):
    def __init__(self, rootPath):
        self.hdrs = []
        w = os.walk(rootPath)
        for tup in w:
            for hdrPath in [fpath for fpath in tup[2] if '.pb.h'==fpath[-5:]]:
                hdrNamespace = os.path.relpath(tup[0],rootPath).split('/')
                self.hdrs.append(ProtoHdr(hdrPath, hdrNamespace))