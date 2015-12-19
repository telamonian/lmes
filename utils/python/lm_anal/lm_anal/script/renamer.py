#!/usr/bin/env python3.4
from argparse import ArgumentParser
import os,sys
from pathlib import Path
import re

DIR = 1
FILE = 2

class RenamerFuncs(object):
    snPat = scientificNotationPat = r'([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)'
    productionDegradationRegex = re.compile(r'production_(%s)_-_degradation_%s' % ((snPat,)*2))

    def productionDegradationToTheta(self, name):
        pdSearch = self.productionDegradationRegex.search(name)
        if pdSearch:
            theta = 'theta_%.1e' % float(pdSearch.group(1))
            return self.productionDegradationRegex.sub(theta, name)
        else:
            return None

    def regexRepl(self, name):
        if self.regex.search(name):
            return self.regex.sub(self.replacePattern, name)
        else:
            return None

class Renamer(RenamerFuncs):
    '''
    walks along the tree pointed to by self.rootPath and renames files using re.sub(self.matchPattern,self.replacePatten,<filename>)
    '''
    walkTupIndex = None

    @property
    def rootPathStr(self):
        return str(self.rootPath)

    def __init__(self, rootPath, matchPattern=None, replacePattern=None, specialFunc=None):
        self.rootPath = Path(rootPath)
        self.matchPattern = matchPattern
        self.replacePattern = replacePattern

        if specialFunc is None:
            self.regex = re.compile(self.matchPattern)
            self.renamerFunc = self.regexRepl
        else:
            self.renamerFunc = self.__getattribute__(specialFunc)
    
    def rename(self, doTest=False):
        walker = os.walk(self.rootPathStr)
        for dirPath,tup in ((Path(tups[0]),tups[self.walkTupIndex]) for tups in walker):
            for name in tup:
                newName = self.renamerFunc(name)
                if newName:
                # if self.regex.search(name):
                #     newName = self.regex.sub(self.replacePattern, name)
                    print('%s >> %s' % (dirPath / name, dirPath / newName))
                    if not doTest:
                        (dirPath / name).rename(dirPath / newName)

class DirRenamer(Renamer):
    walkTupIndex = DIR

class FileRenamer(Renamer):
    walkTupIndex = FILE

if __name__=='__main__':
    '''
    example usage:

    ./renamer.py dir genetic_toggle_switch_test_data '([^\W\d_]+)([\d\.]+)_([^\W\d_]+)([\d\.]+)' '\1_\2_-_\3_\4' test
    ./renamer.py dir gts_-_fflux_-_maxCrossingsZero_-_maxCrossingsN_-_phi_-_theta_-_tiles_-_replicate/ '(mcz_[^_]+)_-_(mcn_[^_]+)_-_(theta_[^_]+)' '\1_-_\2_-_phi1.0e+00_-_\3_-_tiles_12' test
    '''
    parser = ArgumentParser('script for renaming files or directories within a root directory according to a regex/repl or a predefined function')
    parser.add_argument('rootPath', help='path to root dir where files to be renamed are located')
    parser.add_argument('-m', '--matchPattern', help='specify match pattern for regex/repl renamer')
    parser.add_argument('-r', '--replacePattern', help='specify replace pattern for regex/repl renamer')
    parser.add_argument('-s', '--specialFunc', help='name of special function to use for renaming instead of ')
    parser.add_argument('-t', '--doTest', action='store_true', help='if this flag is set, renamer will perform a dry run rather than actually renaming any files.')

    group = parser.add_mutually_exclusive_group()
    group.add_argument('-d', '--dir', action='store_true', help='specify whether you are trying to change the names of directories or files within rootPath')
    group.add_argument('-f', '--file', action='store_true', help='specify whether you are trying to change the names of directories or files within rootPath')

    kwargs = vars(parser.parse_args())

    # renamerTypeName = sys.argv[1].strip()
    # rootPath = sys.argv[2].strip()
    # matchPattern = sys.argv[3]
    # replacePattern = sys.argv[4]

    # try:
    #     doTest = sys.argv[5]
    # except IndexError:
    #     doTest = False
    doTest = kwargs.pop('doTest')
    dirFlag = kwargs.pop('dir')
    fileFlag = kwargs.pop('file')

    if dirFlag:
        RenamerType = DirRenamer
    elif fileFlag:
        RenamerType = FileRenamer
    else:
        raise ValueError("Please set either the -d flag for renaming directories or the -f flag for renaming files")
        # default type if neither --dir nor --file is set is FileRenamer
        # RenamerType = FileRenamer
    
    renamer = RenamerType(**kwargs)     #rootPath=rootPath, matchPattern=matchPattern, replacePattern=replacePattern)
    
    if doTest:
        print('renamerType: %s' % RenamerType.__name__)
        print('rootPath: %s' % kwargs['rootPath'])
        print('matchPattern: %s' % kwargs['matchPattern'])
        print('replacePattern: %s' % kwargs['replacePattern'])

        renamer.rename(doTest=True)
    else:
        renamer.rename()
