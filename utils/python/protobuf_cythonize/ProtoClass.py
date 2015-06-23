from ProtoFunc import ProtoFunc
import os
import re
import sys

class ProtoClass(object):
    def __init__(self, className, classLines, hdrName, namespace):
        self.classLines = classLines
        self.className = className
        self.functions = []
        self.hdrName = hdrName
        self.namespace = namespace
        self.pxd = ['cdef extern from "%s" namespace "%s":\n',
                    ' '*4 + 'cdef cppclass Cpp%s "%s":\n',
                    ' '*8 + 'Cpp%s()\n']
        self.pyx = ['cdef class %s:\n',
                    ' '*4 + 'cdef Cpp%s* thisptr\n',
                    ' '*4 + 'def __cinit__(self):\n',
                    ' '*8 + 'self.thisptr = new Cpp%s()\n',
                    ' '*4 + 'def __dealloc__(self):\n',
                    ' '*8 + 'del self.thisptr\n']
        
        funcLines = []
        for line in self.classLines:
            funcLines.append(line)
            if ';' in line:
                funcLine = ' '.join([line.strip() for line in funcLines])
                self.functions.append(ProtoFunc(funcLine=funcLine))
                funcLines = []
    
    def GenPxd(self):
        self.pxd[0] = self.pxd[0] % (os.path.join(*(self.namespace + [self.hdrName])), '::'.join(self.namespace))
        self.pxd[1] = self.pxd[1] % (self.className, '::'.join(self.namespace + [self.className]))
        self.pxd[2] = self.pxd[2] % (self.className)
        for func in [func for func in self.functions if func.Tag()=='simple']:
            func.GenPxd()
            self.pxd+=func.pxd
            
    def GenPyx(self):
        self.pyx[0] = self.pyx[0] % self.className
        self.pyx[1] = self.pyx[1] % self.className
        self.pyx[3] = self.pyx[3] % self.className
        for func in [func for func in self.functions if func.Tag()=='simple']:
            func.GenPyx()
            self.pyx+=func.pyx