from distutils.version import StrictVersion
from functools import cmp_to_key
import os
import re
from string import Template
import sys

lmLibNames = ('hdf5','libsbml','protobuf','mpich2')

class TplFile(object):
    def __init__(self):
        with open('CMakeConfig.tpl', 'r') as f:
            self.lines = Template(f.read())
        self.subDict = {}
        
    def getLibDir(self, libName, rootLibDir):
        rootTup = os.walk(rootLibDir).next()
        if libName in rootTup[1]:
            libTup = os.walk(os.path.join(rootTup[0],libName)).next()
            if 'include' in libTup[1]:
                return libTup[0]
            else:
                cmp = lambda x, y: StrictVersion(x).__cmp__(y)
                verDirs = sorted(libTup[1], key=cmp_to_key(cmp))
                return os.path.join(libTup[0],verDirs[-1])
        else:
            print 'tried to find install directory of %s in %s, failed' % (libName, rootLibDir)
            raise Error
    
    def setLibDict(self, libName, rootLibDir):
        if libName=='libsbml':
            self.setLibSBMLDict(rootLibDir=rootLibDir)
        elif libName=='mpich2':
            self.setLibMpich2Dict(rootLibDir=rootLibDir)
        else:
            self.setLibDictGeneric(libName=libName,varName=libName.upper() + '_ROOT',rootLibDir=rootLibDir)
    
    def setLibDictGeneric(self, libName, varName, rootLibDir):
        libDirPath = self.getLibDir(libName, rootLibDir)
        self.subDict[varName] = libDirPath
        
    def setLibMpich2Dict(self, rootLibDir):
        libDirPath = self.getLibDir('mpich2', rootLibDir)
        self.subDict['MPI_C_COMPILER'] = os.path.join(libDirPath, 'bin/mpicc')
        self.subDict['MPI_C_EXEC'] = os.path.join(libDirPath, 'bin/mpirun')
        self.subDict['MPI_CXX_COMPILER'] = os.path.join(libDirPath, 'bin/mpicxx')
        self.subDict['MPI_CXX_EXEC'] = os.path.join(libDirPath, 'bin/mpirun')
        
    def setLibSBMLDict(self, rootLibDir):
        libDirPath = self.getLibDir('libsbml', rootLibDir)
        self.subDict['SBML_ROOT'] = libDirPath
    
    def setUseCuda(self, useCuda):
        self.subDict['USE_CUDA'] = useCuda
    
    def setVerbosityLevel(self, verbosityLevel):
        self.subDict['VERBOSITY_LEVEL'] = '%d' % verbosityLevel
    
    def Sub(self):
        self.lines = self.lines.safe_substitute(self.subDict)
    
    def UncommentSubbedLines(self):
        '''
        uncomments any lines in lines which have been substituted (ie no longer have a $ sign). relies on RegEx, so may be wonky
        '''
        self.lines = re.sub('#SET(?!.+[\w].*\s\$)','SET',self.lines)

if __name__=='__main__':
    rootLibDir = sys.argv[1]
    tplFile = TplFile()
    for libName in lmLibNames:
        tplFile.setLibDict(libName, rootLibDir)
    tplFile.setVerbosityLevel(4)
    tplFile.setUseCuda('no')
    tplFile.Sub()
    tplFile.UncommentSubbedLines()
    print tplFile.lines