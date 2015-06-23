from ProtoHdr import ProtoHdr
import re
import sys
    
if __name__=='__main__':
    hdrPath = sys.argv[1]
    namespace = hdrPath.split('/')[-3:-1]
    protoHdr = ProtoHdr(hdrPath, namespace)
    for cls in protoHdr.classes:
        cls.GenPxd()
        cls.GenPyx()
        with open(cls.className+'.pxd','w') as f:
            [f.write(line) for line in cls.pxd]
        with open(cls.className+'.pyx','w') as f:
            [f.write(line) for line in cls.pyx]
#         for func in cls.functions:
#             if func.Tag()=='simple':
#                 print func